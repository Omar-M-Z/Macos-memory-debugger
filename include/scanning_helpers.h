#pragma once

#include <cstddef>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include "util.h"
#include "scan_filter.h"

enum class ScanResult {
    SUCCESS = 0,
    MEM_ALLOC_FAIL = 1,
    MEM_READ_FAIL = 2,
    INVALID_FILTER_CONFIG = 3
};

enum class FilterType {
    SAME,
    CHANGED,
    INCREASED,
    DECREASED,
    NEW_VALUE
};

/**
 * Scans the memory of a process for a specific value of type T.
 * @tparam T the type of the value to scan for
 * @param task the mach port of the target process
 * @param target_value the value to scan for
 * @param results a MemoryObjectStore to store the results of the scan
 * @return a ScanResult indicating the success or failure of the scan
 */
template<typename T>
ScanResult scan_proc_memory_for_value(
    mach_port_t task,
    const T &target_value,
    MemoryObjectStore &results
)
{
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
                    const mach_vm_size_t remaining = region_size - chunk_offset;
                    const mach_vm_size_t searchable_bytes = remaining < chunk_size ? remaining : chunk_size;
                    const mach_vm_size_t requested_bytes = remaining < buffer_size ? remaining : buffer_size;

                    mach_vm_size_t bytes_read;
                    kern_return_t ret = mach_vm_read_overwrite(task, region_start_address + chunk_offset, requested_bytes, (mach_vm_address_t)buffer.get(), &bytes_read);
                    if (ret != KERN_SUCCESS)
                    {
                        return ScanResult::MEM_READ_FAIL;
                    }

                    for (size_t i = 0; i < searchable_bytes && i + target_size <= bytes_read; i += 1)
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

/**
 * Scans the memory of a process for values of type T based on a filter applied to previously found results.
 * @tparam T the type of the value to scan for
 * @param task the mach port of the target process
 * @param input_results a MemoryObjectStore containing previously found results to filter
 * @param results a MemoryObjectStore to store the filtered results of the scan
 * @param filter_type the type of filter to apply (SAME, CHANGED, INCREASED, DECREASED, NEW_VALUE)
 * @param filter_val the value to compare against
 * @return a ScanResult indicating the success or failure of the scan 
 */
template<typename T>
ScanResult scan_proc_memory_for_value_filtered(
    mach_port_t task,
    const MemoryObjectStore &input_results, 
    MemoryObjectStore &results,
    FilterType filter_type,
    const T &filter_val
) {
    results.clear();

    std::unique_ptr<unsigned char[]> buffer(new (std::nothrow) unsigned char[sizeof(T)]);
    if (!buffer) {
        return ScanResult::MEM_ALLOC_FAIL;
    }

    for (const MemoryObject &old_object : input_results.all()) {
        mach_vm_size_t bytes_read = 0;
        kern_return_t ret = mach_vm_read_overwrite(
            task,
            old_object.address,
            sizeof(T),
            (mach_vm_address_t)buffer.get(),
            &bytes_read
        );

        if (ret != KERN_SUCCESS || bytes_read != sizeof(T)) {
            return ScanResult::MEM_READ_FAIL;
        }

        T new_value;
        std::memcpy(&new_value, buffer.get(), sizeof(T));

        T old_value;
        std::memcpy(
            &old_value,
            old_object.bytes.data(),
            sizeof(T)
        );

        bool keep = false;

        switch (filter_type) {
            case FilterType::SAME:
                keep = keep_if_same<T>(old_value, new_value);
                break;

            case FilterType::CHANGED:
                keep = keep_if_changed<T>(old_value, new_value);
                break;

            case FilterType::INCREASED:
                keep = keep_if_increased<T>(old_value, new_value);
                break;

            case FilterType::DECREASED:
                keep = keep_if_decreased<T>(old_value, new_value);
                break;

            case FilterType::NEW_VALUE:
                keep = keep_if_new_value<T>(new_value, filter_val);
                break;

            default:
                return ScanResult::INVALID_FILTER_CONFIG;
        }

        if (keep) {
            results.add(old_object.address, buffer.get(), sizeof(T));
        }
    }

    return ScanResult::SUCCESS;
}
