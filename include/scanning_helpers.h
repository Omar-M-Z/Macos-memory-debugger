#pragma once

#include <cstddef>
#include <mach/mach.h>
#include <mach/mach_vm.h>

namespace scan_filter {
    enum class FilterType;
}

enum class ScanResult;
struct MemoryObject;
class MemoryObjectStore;

/**
 * Scans a process's readable memory regions for a target value.
 * 
 * @param task the Mach task port for the target process
 * @param target_bytes the bytes to search for
 * @param target_size the size of the bytes to search for
 * @param results storage for matching addresses and bytes
 * @return 0 on success, non-zero on failure
 */
ScanResult scan_proc_memory_for_bytes(
    mach_port_t task,
    const unsigned char *target_bytes,
    size_t target_size,
    MemoryObjectStore &results
);

/**
 * Scans a process's readable memory regions for a target value, applying a filter to the results.
 *
 * @param task the Mach task port for the target process
 * @param input_results the results from a previous scan to filter
 * @param target_bytes the bytes to search for
 * @param target_size the size of the bytes to search for
 * @param results storage for matching addresses and bytes
 * @param filter the filter to apply
 * @return 0 on success, non-zero on failure
 */
ScanResult scan_proc_memory_for_bytes_filtered(
    mach_port_t task,
    const MemoryObjectStore &input_results,
    const unsigned char *target_bytes,
    size_t target_size,
    MemoryObjectStore &results,
    scan_filter::FilterType filter
);
