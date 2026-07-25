#pragma once
#include <cstddef>
#include <cstring>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "scanning_helpers.h"

inline const char* COLOR_DEFAULT = "\033[0m";
inline const char* COLOR_GREEN = "\033[32m";
inline const char* COLOR_RED = "\033[31m";


enum class ErrorType {
    USAGE,
    OTHER
};

enum class ScanResult {
    SUCCESS = 0,
    MEM_ALLOC_FAIL = 1,
    MEM_READ_FAIL = 2
};

/**
 * Outputs a general informational message (default terminal text color)
 * @param message the message to output
 */
void log_message(const std::string &message);

/**
 * Outputs a success message (green text)
 * @param message the success message to output
 */
void log_success(const std::string &message);

/**
 * Outputs an error message (red text)
 * @param error_type the type of error (USAGE, OTHER)
 * @param message the error message to output
 */
void log_error(ErrorType error_type, const std::string &message);

struct MemoryObjectStore;

/**
 * Represents a memory object found during scanning, consisting of an address and the bytes at that address.
 */
struct MemoryObject {
    mach_vm_address_t address;
    std::vector<unsigned char> bytes;
};

/**
 * A store for memory objects found during scanning, allowing adding new objects and retrieving them.
 */
class MemoryObjectStore {
public:
    /**
     * Adds a memory object to the store.
     * @param address the address of the memory object
     * @param object a pointer to the object data
     * @param object_size the size of the object data
     */
    void add(mach_vm_address_t address, const void *object, size_t object_size);

    /**
     * Clears all memory objects from the store.
     */
    void clear();
    /**
     * Checks if the store is empty.
     * @return true if the store is empty, false otherwise
     */
    bool empty() const;
    /**
     * Gets the number of memory objects in the store.
     * @return the number of memory objects
     */
    size_t size() const;

    /**
     * Gets a memory object at a specific index.
     * @param index the index of the memory object to retrieve
     * @return a reference to the memory object
     */ 
    const MemoryObject &at(size_t index) const;
    
    /**
     * Gets all memory objects in the store.
     * @return a reference to the vector of memory objects
     */
    const std::vector<MemoryObject> &all() const;

    /**
     * Converts a memory object at a specific index to a value of the requested type.
     * @param index the index of the memory object to convert
     * @return the converted value
     */
    template<typename T>
    T value_as(size_t index) const
    {
        const MemoryObject &object = at(index);
        if (object.bytes.size() != sizeof(T)) {
            throw std::runtime_error("Stored object has a different size than requested type.");
        }

        T value;
        std::memcpy(&value, object.bytes.data(), sizeof(T));
        return value;
    }

private:
    std::vector<MemoryObject> objects;
};

/**
 * Scans a process's readable memory regions for a target value.
 * @param task the Mach task port for the target process
 * @param target_value the value to search for
 * @param results storage for matching addresses and bytes
 * @return 0 on success, non-zero on failure
 */
template<typename T>
ScanResult scan_proc_memory_for_value(mach_port_t task, const T &target_value, MemoryObjectStore &results)
{
    return scan_proc_memory_for_bytes(
        task,
        reinterpret_cast<const unsigned char *>(&target_value),
        sizeof(T),
        results
    );
}
