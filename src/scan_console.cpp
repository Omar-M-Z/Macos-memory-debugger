#include <iostream>
#include <vector>
#include <stdexcept>

#include "scan_console.h"
#include "scan_filter.h"
#include "scanning_helpers.h"
#include "util.h"

// creating a new scan console and performing an initial scan based on the provided arguments.
// arg1 = value to scan for, arg2 = type (int/float/double/byte)
// scan results are saved in an internal object
scan_console::scan_console(debugger_console &parent, const std::vector<std::string> &args) : sub_console(parent)
{
    if (args.size() < 3) {
        log_error(ErrorType::USAGE, "Usage: scan <value> <type>. Types: int, float, double, byte");
        throw std::invalid_argument("");
        return;
    }

    this->target_value_str = args[1];
    this->scan_type = args[2];

    // Perform initial scan based on type

    ScanResult result;
    if (this->scan_type == "int") {
        try {
            int value = std::stoi(this->target_value_str);
            result = scan_proc_memory_for_value<int>(parent.task, value, this->scan_results);
        } catch (const std::exception &e) {
            log_error(ErrorType::USAGE, "Invalid value for type int.");
            throw std::invalid_argument("");
            return;
        }
    } else if (this->scan_type == "float") {
        try {
            float value = std::stof(this->target_value_str);
            result = scan_proc_memory_for_value<float>(parent.task, value, this->scan_results);
        } catch (const std::exception &e) {
            log_error(ErrorType::USAGE, "Invalid value for type float.");
            throw std::invalid_argument("");
            return;
        }
    } else if (this->scan_type == "double") {
        try {
            double value = std::stod(this->target_value_str);
            result = scan_proc_memory_for_value<double>(parent.task, value, this->scan_results);
        } catch (const std::exception &e) {
            log_error(ErrorType::USAGE, "Invalid value for type double.");
            throw std::invalid_argument("");
            return;
        }
    } else if (this->scan_type == "rawbyte") {
        unsigned char value;
        if (!parse_rawbyte(this->target_value_str, value)) {
            log_error(ErrorType::USAGE, "Invalid value for type rawbyte.");
            throw std::invalid_argument("");
            return;
        }
        result = scan_proc_memory_for_value<unsigned char>(parent.task, value, this->scan_results);

    } else if (this->scan_type == "hexbyte") {
        unsigned char value;
        if (!parse_hexbyte(this->target_value_str, value)) {
            log_error(ErrorType::USAGE, "Invalid value for type hexbyte.");
            throw std::invalid_argument("");
            return;
        }
        result = scan_proc_memory_for_value<unsigned char>(parent.task, value, this->scan_results);
    }
    else {
        log_error(ErrorType::USAGE, "Invalid type specified for scan. Use int, float, double, rawbyte, or hexbyte.");
        throw std::invalid_argument("");
        return;
    }
    if (result != ScanResult::SUCCESS) {
        log_error(ErrorType::OTHER, "Memory scan failed with error code: " + std::to_string(static_cast<int>(result)));
        throw std::runtime_error("");
        return;
    }
    log_message("Initial scan complete. Found " + std::to_string(this->scan_results.size()) + " matches.");
    
}

// after the initial command that started the scan and created the scan console, this function will handle any additional commands (until the user exits the scan)
void scan_console::handle_command(const std::vector<std::string> &args)
{
    if (args.empty()) {
        log_message("No command entered.");
        return;
    }

    else if (args[0] == "list") {
        log_message("Listing found addresses:");
        print_scan_results();
        return;
    }

    else if (args[0] == "refine") {
        if (this->scan_results.empty()) {
            log_message("No scan results to refine.");
            return;
        }
        if (args.size() < 2) {
            log_error(ErrorType::USAGE, "Usage: refine <difference|same|increased|decreased> <value>");
            return;
        }

        FilterType filter_type;

        if (args[1] == "same") {
            filter_type = FilterType::SAME;
        } else if (args[1] == "changed") {
            filter_type = FilterType::CHANGED;
        } else if (args[1] == "increased") {
            filter_type = FilterType::INCREASED;
        } else if (args[1] == "decreased") {
            filter_type = FilterType::DECREASED;
        } else if (args[1] == "new_value") {
            filter_type = FilterType::NEW_VALUE;
        } else {
            log_error(ErrorType::USAGE, "Invalid refine type. Use same, changed, increased, decreased, or new_value.");
            return;
        }

        MemoryObjectStore new_results;

        try {
            ScanResult result;

            if (scan_type == "int") {
                result = scan_proc_memory_for_value_filtered<int>(parent.task, scan_results, new_results, filter_type, std::stoi(args[2]));
            } else if (scan_type == "float") {
                result = scan_proc_memory_for_value_filtered<float>(parent.task, scan_results, new_results,filter_type, std::stof(args[2]));
            } else if (scan_type == "double") {
                result = scan_proc_memory_for_value_filtered<double>(parent.task, scan_results, new_results,filter_type,std::stod(args[2]));
            } else if (scan_type == "rawbyte" || scan_type == "hexbyte") {
                log_message("Refinement for rawbyte and hexbyte types is not yet implemented.");
                // TODO: Implement refinement for rawbyte and hexbyte types
            }
        } catch (const std::exception&) {
            log_error(ErrorType::USAGE, "Invalid refinement value.");
            return;
        }
        
        return;
    }

    else if (args[0] == "help") {
        print_help();
        return;
    }

    else if (args[0] == "exit") {
        log_message("Exiting scan console.");
        this->parent.remove_active_sub_console();
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

std::string scan_console::get_prompt() const
{
    return this->prompt;
}

// default destructor
scan_console::~scan_console() = default;
