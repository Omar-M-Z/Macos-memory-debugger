#include "util.h"
#include <iostream>
#include <memory>
#include <new>


// MESSAGE LOGGING

void log_message(const std::string &message)
{
    std::cout << COLOR_DEFAULT << message << COLOR_DEFAULT << std::endl;
}

void log_success(const std::string &message)
{
    std::cout << COLOR_GREEN << message << COLOR_DEFAULT << std::endl;
}

void log_error(ErrorType error_type, const std::string &message)
{
    std::string error_prefix;
    if (error_type == ErrorType::USAGE) {
        error_prefix = "Usage: ";
    } else {
        error_prefix = "Error: ";
    }
    std::cerr << COLOR_RED << error_prefix << " " << message << COLOR_DEFAULT << std::endl;
}

// SCAN STORAGE 

void MemoryObjectStore::add(mach_vm_address_t address, const void *object, size_t object_size)
{
    const unsigned char *bytes = static_cast<const unsigned char *>(object);
    objects.push_back({address, std::vector<unsigned char>(bytes, bytes + object_size)});
}

void MemoryObjectStore::clear()
{
    objects.clear();
}

bool MemoryObjectStore::empty() const
{
    return objects.empty();
}

size_t MemoryObjectStore::size() const
{
    return objects.size();
}

const MemoryObject &MemoryObjectStore::at(size_t index) const
{
    return objects.at(index);
}

const std::vector<MemoryObject> &MemoryObjectStore::all() const
{
    return objects;
}
