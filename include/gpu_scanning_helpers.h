#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>

#include "metal_resources.h"
#include "scanning_helpers.h"

struct ScanMemoryParameters {
    uint32_t bytes_read;          // Bytes available in the input buffer, including overlap.
    uint32_t searchable_bytes;    // Candidate addresses must start within this many bytes.
    uint32_t target_size;         // Number of bytes to compare at each candidate address.
    uint32_t target_alignment;    // Distance between candidate addresses: alignof(T).
    uint32_t aligned_start_index; // Offset of the first correctly aligned address.
};

/**
 * same memory scanning logic as scan_proc_memory_for_value<T>, but uses the GPU to perform the scan.
 */
template<typename T>
ScanResult scan_proc_memory_for_value_gpu(mach_port_t task, const T& target_value,
    MemoryObjectStore& results, const MetalResources& metal, uint64_t *bytes_searched = nullptr)
{
    if (bytes_searched) *bytes_searched = 0;
    if (!metal.device || !metal.scan_pipeline_state || !metal.command_queue) {
        log_error(ErrorType::OTHER, "Metal resources were not properly initialized.");
        return ScanResult::GPU_FAIL;
    }
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    const unsigned char *target_bytes = reinterpret_cast<const unsigned char *>(&target_value);
    size_t target_size = sizeof(T);

    mach_vm_address_t region_start_address = 0;
    mach_vm_size_t region_size;
    vm_region_basic_info_data_64_t region_info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object_name = MACH_PORT_NULL;

    while (mach_vm_region(task, &region_start_address, &region_size, VM_REGION_BASIC_INFO_64, (vm_region_info_t)&region_info, &count, &object_name) == KERN_SUCCESS)
    {
        if (region_info.protection & VM_PROT_READ)
        {

            constexpr mach_vm_size_t chunk_size = 16 * 1024 * 1024;
            const mach_vm_size_t overlap = target_size > 0 ? target_size - 1 : 0;
            const mach_vm_size_t buffer_size = chunk_size + overlap;
            std::unique_ptr<unsigned char[]> buffer(new (std::nothrow) unsigned char[buffer_size]);
            if (buffer)
            {
                for (mach_vm_size_t chunk_offset = 0; chunk_offset < region_size; chunk_offset += chunk_size)
                {
                    // how much still hasnt been processed in this region
                    const mach_vm_size_t remaining = region_size - chunk_offset;
                    // how much of the remaining bytes can we start scanning for target values
                    const mach_vm_size_t searchable_bytes = remaining < chunk_size ? remaining : chunk_size;
                    // number of bytes actually read into the buffer, including overlap bytes (the chunk size itself does not include this)
                    const mach_vm_size_t requested_bytes = remaining < buffer_size ? remaining : buffer_size;

                    mach_vm_size_t bytes_read;
                    kern_return_t ret = mach_vm_read_overwrite(task, region_start_address + chunk_offset, requested_bytes, (mach_vm_address_t)buffer.get(), &bytes_read);
                    if (ret != KERN_SUCCESS)
                    {
                        continue;
                    }
                    if (bytes_searched) {
                        *bytes_searched += std::min<mach_vm_size_t>(bytes_read, searchable_bytes);
                    }

                    // ensuring the scan starts at an address of the correct alignment for type T
                    mach_vm_address_t aligned_start_address = region_start_address + chunk_offset;
                    if (aligned_start_address % alignof(T) != 0) {
                        aligned_start_address += alignof(T) - (aligned_start_address % alignof(T));
                    }
                    size_t aligned_start_index = aligned_start_address - (region_start_address + chunk_offset);


                    // Skip when the read cannot contain one complete value, or when
                    // the first correctly aligned offset is outside this chunk.
                    if (bytes_read < target_size || aligned_start_index >= searchable_bytes) {
                        continue;
                    }

                    // last index in the chunk in which a target value can start
                    const size_t last_start = std::min<size_t>(searchable_bytes - 1, bytes_read - target_size);

                    // No aligned candidate exists before the final valid start.
                    if (aligned_start_index > last_start) {
                        continue;
                    }

                    const size_t candidates = 1 + (last_start - aligned_start_index) / alignof(T);
                    const ScanMemoryParameters parameters{
                        static_cast<uint32_t>(bytes_read), static_cast<uint32_t>(searchable_bytes),
                        static_cast<uint32_t>(target_size), static_cast<uint32_t>(alignof(T)),
                        static_cast<uint32_t>(aligned_start_index)};

                    // Copy this process-memory chunk into a shared Metal buffer.
                    // The GPU writes one byte to matches for each candidate address.
                    MTL::Buffer* input = metal.device->newBuffer(buffer.get(), bytes_read, MTL::ResourceStorageModeShared);
                    MTL::Buffer* matches = metal.device->newBuffer(candidates, MTL::ResourceStorageModeShared);

                    // A command buffer holds this chunk's GPU work. The compute
                    // encoder records the pipeline, its inputs, and the dispatch.
                    MTL::CommandBuffer* command = metal.command_queue->commandBuffer();
                    MTL::ComputeCommandEncoder* encoder = command ? command->computeCommandEncoder() : nullptr;
                    if (!input || !matches || !encoder) {
                        if (encoder) encoder->endEncoding();
                        if (input) input->release();
                        if (matches) matches->release();
                        pool->release();
                        return ScanResult::GPU_FAIL;
                    }

                    // Select scan_memory_for_value and bind its four arguments:
                    // chunk, target value, scan parameters, and output flags.
                    encoder->setComputePipelineState(metal.scan_pipeline_state);
                    encoder->setBuffer(input, 0, 0);
                    encoder->setBytes(target_bytes, target_size, 1);
                    encoder->setBytes(&parameters, sizeof(parameters), 2);
                    encoder->setBuffer(matches, 0, 3);

                    // Launch one GPU thread per candidate address. Threads are
                    // grouped in batches that fit the pipeline's hardware limit.
                    const NS::UInteger group_size = std::min<NS::UInteger>(256, metal.scan_pipeline_state->maxTotalThreadsPerThreadgroup());
                    encoder->dispatchThreads(MTL::Size::Make(candidates, 1, 1), MTL::Size::Make(group_size, 1, 1));
                    encoder->endEncoding();

                    // Submit the recorded work and wait before reading matches.
                    command->commit();
                    command->waitUntilCompleted();
                    if (command->status() != MTL::CommandBufferStatusCompleted) {
                        input->release();
                        matches->release();
                        pool->release();
                        return ScanResult::GPU_FAIL;
                    }

                    // Shared buffers are CPU-visible after completion. Convert
                    // each set flag back to its process address and store it.
                    const auto* flags = static_cast<const unsigned char*>(matches->contents());
                    for (size_t i = 0; i < candidates; ++i) {
                        if (flags[i]) {
                            const size_t offset = aligned_start_index + i * alignof(T);
                            results.add(region_start_address + chunk_offset + offset, buffer.get() + offset, target_size);
                        }
                    }
                    input->release();
                    matches->release();

                }
            } else {
                pool->release();
                return ScanResult::MEM_ALLOC_FAIL;
            }
        }
        region_start_address += region_size;
        count = VM_REGION_BASIC_INFO_COUNT_64;
    }

    pool->release();
    return ScanResult::SUCCESS;

}
