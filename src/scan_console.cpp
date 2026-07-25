#include <iostream>
#include <vector>

#include "scan_console.h"
#include "scan_filter.h"
#include "util.h"

// creating a new scan console and performing an initial scan based on the provided arguments.
// arg1 = value to scan for, arg2 = type (int/float/double/byte)
// scan results are saved in an internal object
scan_console::scan_console(debugger_console &parent, const std::vector<std::string> &args) : sub_console(parent)
{
    if (args.size() < 2) {
        std::cerr << "Usage: scan <value> <type>" << std::endl;
        std::cerr << "Types: int, float, double, byte" << std::endl;
        return;
    }

    this->target_value_str = args[0];
    this->scan_type = args[1];

    // Perform initial scan based on type
    try {
        if (this->scan_type == "int") {
            try {
                int value = std::stoi(this->target_value_str);
                scan_proc_memory_for_value<int>(parent.task, value, this->scan_results);
            } catch (const std::exception &e) {
                log_error(ErrorType::USAGE, "Invalid value for type int.");
                return;
            }
        } else if (this->scan_type == "float") {
            try {
                float value = std::stof(this->target_value_str);
                scan_proc_memory_for_value<float>(parent.task, value, this->scan_results);
            } catch (const std::exception &e) {
                log_error(ErrorType::USAGE, "Invalid value for type float.");
                return;
            }
        } else if (this->scan_type == "double") {
            try {
                double value = std::stod(this->target_value_str);
                scan_proc_memory_for_value<double>(parent.task, value, this->scan_results);
            } catch (const std::exception &e) {
                log_error(ErrorType::USAGE, "Invalid value for type double.");
                return;
            }
        } else if (this->scan_type == "rawbyte") {
            if (this->target_value_str.size() != 8) {
                log_error(ErrorType::USAGE, "Invalid value for type rawbyte.");
                return;
            }

            unsigned char value = 0;
            for (char bit : this->target_value_str) {
                if (bit != '0' && bit != '1') {
                    log_error(ErrorType::USAGE, "Invalid value for type rawbyte.");
                    return;
                }
                value = static_cast<unsigned char>((value << 1) | (bit - '0'));
            }
            scan_proc_memory_for_value<unsigned char>(parent.task, value, this->scan_results);

        } else if (this->scan_type == "hexbyte") {

            if (this->target_value_str.empty()) {
                log_error(ErrorType::USAGE, "Invalid value for type hexbyte.");
                return;
            }

            unsigned int parsed = 0;
            for (char c : this->target_value_str) {
                unsigned int digit;
                if (c >= '0' && c <= '9') {
                    digit = c - '0';
                } else if (c >= 'a' && c <= 'f') {
                    digit = c - 'a' + 10;
                } else if (c >= 'A' && c <= 'F') {
                    digit = c - 'A' + 10;
                } else {
                    log_error(ErrorType::USAGE, "Invalid value for type hexbyte.");
                    return;
                }

                parsed = (parsed << 4) | digit;
                if (parsed > 0xff) {
                    log_error(ErrorType::USAGE, "Invalid value for type hexbyte.");
                    return;
                }
            }

            scan_proc_memory_for_value<unsigned char>(parent.task, static_cast<unsigned char>(parsed), this->scan_results);
        }
        else {
            log_error(ErrorType::USAGE, "Invalid type specified for scan. Use int, float, double, rawbyte, or hexbyte.");
            return;
        }
        log_message("Initial scan complete. Found " + std::to_string(this->scan_results.size()) + " matches.");
    } catch (const std::exception &e) {
        log_error(ErrorType::OTHER, "Error during scan: " + std::string(e.what()));
    }
    
}

// after the initial command that started the scan and created the scan console, this function will handle any additional commands (until the user exits the scan)
void scan_console::handle_command(const std::vector<std::string> &args)
{
    if (args.empty()) {
        log_message("No command entered.");
        return;
    }

    if (args[0] == "list") {
        log_message("Listing found addresses:");
        print_scan_results();
        return;
    }

    if (args[0] == "refine") {
        if (this->scan_results.empty()) {
            log_message("No scan results to refine.");
            return;
        }
        if (args.size() < 2) {
            log_error(ErrorType::USAGE, "Usage: refine <difference|same|increased|decreased> <value>");
            return;
        }

        // TODO: refine the scan results based on the new value or filter type

        return;
    }

    if (args[0] == "help") {
        print_help();
        return;
    }

    log_message("Command " + args[0] + " not recognized");
}

void scan_console::print_available_commands() const
{
    std::cout << "Available commands:" << std::endl;
    std::cout << "  list - show all found addresses" << std::endl;
    std::cout << "  refine <value> - scan for a new value within current results" << std::endl;
    std::cout << "  help - show this help message" << std::endl;
}

void scan_console::print_scan_results() const
{
    if (this->scan_results.empty()) {
        std::cout << "No results from scan." << std::endl;
        return;
    }

    std::cout << "Found " << this->scan_results.size() << " addresses:" << std::endl;
    for (size_t i = 0; i < this->scan_results.size() && i < 100; ++i) {
        printf("  0x%llx\n", this->scan_results.at(i).address);
    }
    if (this->scan_results.size() > 100) {
        std::cout << "  ... and " << (this->scan_results.size() - 100) << " more" << std::endl;
    }
}

void scan_console::print_help() const
{
    std::cout << "Scan Console Commands:" << std::endl;
    std::cout << "  list - show all found addresses" << std::endl;
    std::cout << "  refine <value> - refine search with a new value" << std::endl;
    std::cout << "  help - show this help message" << std::endl;
}

// default destructor
scan_console::~scan_console() = default;
