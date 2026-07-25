#include "scan_filter.h"

namespace scan_filter {

static bool is_ordered_scan_type(const std::string &scan_type)
{
    return scan_type == "int" || scan_type == "float" || scan_type == "double" ||
        scan_type == "rawbyte" || scan_type == "hexbyte";
}

static bool keep_if_same_bytes(const MemoryObject &old_object, const MemoryObject &new_object)
{
    return old_object.bytes == new_object.bytes;
}

static bool keep_if_bytes_changed(const MemoryObject &old_object, const MemoryObject &new_object)
{
    return old_object.bytes != new_object.bytes;
}

static bool keep_if_increased(const MemoryObject &old_object, const MemoryObject &new_object)
{
    if (old_object.bytes.size() != new_object.bytes.size()) {
        return false;
    }
    return old_object.bytes < new_object.bytes;
}

static bool keep_if_decreased(const MemoryObject &old_object, const MemoryObject &new_object)
{
    if (old_object.bytes.size() != new_object.bytes.size()) {
        return false;
    }
    return old_object.bytes > new_object.bytes;
}

bool verify_scan_filter(const std::string &scan_type, FilterType filter)
{
    switch (filter) {
        case FilterType::Same:
        case FilterType::Changed:
            return true;
        case FilterType::Increased:
        case FilterType::Decreased:
            return is_ordered_scan_type(scan_type);
    }
    return false;
}

bool apply_filter(const MemoryObject &old_object, const MemoryObject &new_object, FilterType filter)
{
    switch (filter) {
        case FilterType::Same:
            return keep_if_same_bytes(old_object, new_object);
        case FilterType::Changed:
            return keep_if_bytes_changed(old_object, new_object);
        case FilterType::Increased:
            return keep_if_increased(old_object, new_object);
        case FilterType::Decreased:
            return keep_if_decreased(old_object, new_object);
    }
    return false;
}

} // namespace scan_filter
