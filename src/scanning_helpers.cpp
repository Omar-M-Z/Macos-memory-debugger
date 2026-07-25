#include "scanning_helpers.h"
#include "util.h"
#include "scan_filter.h"
#include <iostream>
#include <memory>

ScanResult scan_proc_memory_for_bytes(
    mach_port_t task,
    const unsigned char *target_bytes,
    size_t target_size,
    MemoryObjectStore &results
)
{
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
                    const mach_vm_size_t remaining = region_size - chunk_offset;
                    const mach_vm_size_t searchable_bytes = remaining < chunk_size ? remaining : chunk_size;
                    const mach_vm_size_t requested_bytes = remaining < buffer_size ? remaining : buffer_size;

                    mach_vm_size_t bytes_read;
                    kern_return_t ret = mach_vm_read_overwrite(task, region_start_address + chunk_offset, requested_bytes, (mach_vm_address_t)buffer.get(), &bytes_read);
                    if (ret != KERN_SUCCESS)
                    {
                        return ScanResult::MEM_READ_FAIL;
                    }

                    for (size_t i = 0; i < searchable_bytes && i + target_size <= bytes_read; i += 4)
                    {
                        if (std::memcmp(buffer.get() + i, target_bytes, target_size) == 0){
                            results.add(region_start_address + chunk_offset + i, buffer.get() + i, target_size);
                        }
                    }
                }
            } else {
                return ScanResult::MEM_ALLOC_FAIL;
            }
        }
        region_start_address += region_size;
        count = VM_REGION_BASIC_INFO_COUNT_64;
    }

    return ScanResult::SUCCESS;
}

ScanResult scan_proc_memory_for_bytes_filtered(
    mach_port_t task,
    const MemoryObjectStore &input_results,
    const unsigned char *target_bytes,
    size_t target_size,
    MemoryObjectStore &results,
    scan_filter::FilterType filter
)
{
    results.clear();

    std::unique_ptr<unsigned char[]> buffer(new (std::nothrow) unsigned char[target_size]);
    if (!buffer) {
        return ScanResult::MEM_ALLOC_FAIL;
    }

    for (const MemoryObject &old_object : input_results.all()) {
        mach_vm_size_t bytes_read = 0;
        kern_return_t ret = mach_vm_read_overwrite(
            task,
            old_object.address,
            target_size,
            (mach_vm_address_t)buffer.get(),
            &bytes_read
        );

        if (ret != KERN_SUCCESS || bytes_read != target_size) {
            return ScanResult::MEM_READ_FAIL;
        }

        if (std::memcmp(buffer.get(), target_bytes, target_size) != 0) {
            continue;
        }

        std::vector<unsigned char> new_bytes(buffer.get(), buffer.get() + target_size);
        MemoryObject new_object{old_object.address, std::move(new_bytes)};

        if (scan_filter::apply_filter(old_object, new_object, filter)) {
            results.add(new_object.address, new_object.bytes.data(), new_object.bytes.size());
        }
    }

    return ScanResult::SUCCESS;
}
