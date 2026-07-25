#include "util.h"
#include <iostream>
#include <memory>
#include <new>


// MESSAGE LOGGING

/**
 * Outputs a general informational message (default terminal text color)
 * @param message the message to output
 */
void log_message(const std::string &message)
{
    std::cout << COLOR_DEFAULT << message << COLOR_DEFAULT << std::endl;
}

/**
 * Outputs a success message (green text)
 * @param message the success message to output
 */
void log_success(const std::string &message)
{
    std::cout << COLOR_GREEN << message << COLOR_DEFAULT << std::endl;
}

/**
 * Outputs an error message (red text)
 * @param error_type the type of error (USAGE, OTHER)
 * @param message the error message to output
 */
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

/**
 * Adds a memory object to the store.
 * @param address the address of the memory object
 * @param object a pointer to the memory object
 * @param object_size the size of the memory object
 */
void MemoryObjectStore::add(mach_vm_address_t address, const void *object, size_t object_size)
{
    const unsigned char *bytes = static_cast<const unsigned char *>(object);
    objects.push_back({address, std::vector<unsigned char>(bytes, bytes + object_size)});
}

/**
 * Clears all memory objects from the store.
 */
void MemoryObjectStore::clear()
{
    objects.clear();
}

/**
 * Checks if the store is empty.
 * @return true if the store is empty, false otherwise
 */
bool MemoryObjectStore::empty() const
{
    return objects.empty();
}

/**
 * Returns the number of memory objects in the store.
 * @return the number of memory objects
 */
size_t MemoryObjectStore::size() const
{
    return objects.size();
}

/**
 * Returns the memory object at the specified index.
 * @param index the index of the memory object to return
 * @return a reference to the memory object at the specified index
 */
const MemoryObject &MemoryObjectStore::at(size_t index) const
{
    return objects.at(index);
}

/**
 * Returns a reference to the vector of all memory objects.
 * @return a reference to the vector of all memory objects
 */
const std::vector<MemoryObject> &MemoryObjectStore::all() const
{
    return objects;
}


// ARGUMENT PARSING AND VALIDATION

/**
 * Parses exactly eight binary digits into a byte.
 * @param input the binary text to parse
 * @param value receives the parsed byte when parsing succeeds
 * @return true when input is valid, false otherwise
 */
bool parse_rawbyte(const std::string &input, unsigned char &value)
{
    if (input.size() != 8) {
        return false;
    }

    unsigned char parsed = 0;
    for (char bit : input) {
        if (bit != '0' && bit != '1') {
            return false;
        }
        parsed = static_cast<unsigned char>((parsed << 1) | (bit - '0'));
    }

    value = parsed;
    return true;
}

/**
 * Parses a hexadecimal string into a byte.
 * @param input the hexadecimal text to parse
 * @param value receives the parsed byte when parsing succeeds
 * @return true when input is valid, false otherwise
 */
bool parse_hexbyte(const std::string &input, unsigned char &value)
{
    if (input.empty() || input.size() > 2) {
        return false;
    }

    unsigned int parsed = 0;
    for (char c : input) {
        unsigned int digit;
        if (c >= '0' && c <= '9') {
            digit = static_cast<unsigned int>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = static_cast<unsigned int>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = static_cast<unsigned int>(c - 'A' + 10);
        } else {
            return false;
        }

        parsed = (parsed << 4) | digit;
    }

    value = static_cast<unsigned char>(parsed);
    return true;
}
