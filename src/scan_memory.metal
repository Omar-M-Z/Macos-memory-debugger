#include <metal_stdlib>

using namespace metal;


struct ScanMemoryParameters {
    uint bytes_read;
    uint searchable_bytes;
    uint target_size;
    uint target_alignment;
    uint aligned_start_index;
};

kernel void scan_memory_for_value(
    const device uchar* chunk,
    const device uchar* target,
    constant ScanMemoryParameters& parameters,
    device uchar* matches,
    uint index [[thread_position_in_grid]])
{
    if (parameters.target_alignment == 0 ||
        parameters.aligned_start_index >= parameters.searchable_bytes) {
        return;
    }

    const uint candidate_count = 1 +
        (parameters.searchable_bytes - 1 - parameters.aligned_start_index) /
        parameters.target_alignment;
    if (index >= candidate_count) {
        return;
    }

    const uint offset = parameters.aligned_start_index +
        index * parameters.target_alignment;
    matches[index] = 0;
    if (parameters.target_size == 0 || offset >= parameters.bytes_read ||
        parameters.target_size > parameters.bytes_read - offset) {
        return;
    }

    for (uint byte = 0; byte < parameters.target_size; ++byte) {
        if (chunk[offset + byte] != target[byte]) {
            return;
        }
    }
    matches[index] = 1;
}
