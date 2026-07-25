#pragma once

#include <string>

#include "util.h"

namespace scan_filter {

enum class FilterType {
    Same,
    Changed,
    Increased,
    Decreased
};

/**
 * Verify whether the given scan type supports the requested filter.
 * This is useful when some comparisons only make sense for numeric or ordered types.
 *
 * @param scan_type the type of scan being performed
 * @param filter the filter to verify
 * @return true if the filter is supported for the given scan type, false otherwise
 */
bool verify_scan_filter(const std::string &scan_type, FilterType filter);

/**
 * Apply the named filter to an old/new scan object pair.
 * Returns true if the pair passes the requested filter.
 *
 * @param old_object the previous memory object
 * @param new_object the current memory object
 * @param filter the filter to apply
 * @return true if the pair passes the filter, false otherwise
 */
bool apply_filter(const MemoryObject &old_object, const MemoryObject &new_object, FilterType filter);

}
