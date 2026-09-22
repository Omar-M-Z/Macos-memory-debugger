#include "scan_commands.h"

#include <chrono>
#include <cstdio>
#include <exception>
#include <iostream>

#include "console_system.h"
#include "gpu_scanning_helpers.h"
#include "scan_filter.h"
#include "scanning_helpers.h"
#include "util.h"

void handle_scan_command(debugger_console &console, const std::vector<std::string> &args)
{
    if (args.size() < 2) {
        log_error(ErrorType::USAGE, "scan <store-name> -v <value> -t <type> [-gpu] [-time] [-bytes]");
        return;
    }

    const std::string &name = args[1];
    const std::vector<CommandOptionInput> allowed_options = {
        {"-v", true}, {"-t", true}, {"-gpu", false}, {"-time", false}, {"-bytes", false}
    };
    const CommandOptionParsed options = parse_options(args, 2, allowed_options);
    if (!options || !options->count("-v") || !options->count("-t")) {
        log_error(ErrorType::USAGE, "scan <store-name> -v <value> -t <type> [-gpu] [-time] [-bytes]");
        return;
    }
    const std::string &value = options->at("-v");
    const std::string &type = options->at("-t");
    const bool use_gpu = options->count("-gpu");
    const bool show_time = options->count("-time");
    const bool show_bytes = options->count("-bytes");

    if (console.scan_stores.find(name) != console.scan_stores.end()) {
        log_error(ErrorType::OTHER, "A scan store named '" + name + "' already exists.");
        return;
    }

    StoredScan stored{MemoryObjectStore(name), type};
    ScanResult result;
    double scan_time_ms = 0;
    uint64_t bytes_searched = 0;
    const auto scan = [&](const auto& target) {
        const auto start = std::chrono::steady_clock::now();
        ScanResult scan_result;
        if (use_gpu) {
            scan_result = console.metal_resources
                ? scan_proc_memory_for_value_gpu(console.task, target, stored.objects,
                    *console.metal_resources, &bytes_searched)
                : ScanResult::GPU_FAIL;
        } else {
            scan_result = scan_proc_memory_for_value(console.task, target, stored.objects, &bytes_searched);
        }
        scan_time_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        return scan_result;
    };

    try {
        if (type == "int") {
            result = scan(std::stoi(value));
        } else if (type == "float") {
            result = scan(std::stof(value));
        } else if (type == "double") {
            result = scan(std::stod(value));
        } else if (type == "rawbyte") {
            unsigned char byte;
            if (!parse_rawbyte(value, byte)) {
                throw std::invalid_argument("invalid byte");
            }
            result = scan(byte);
        } else if (type == "hexbyte") {
            unsigned char byte;
            if (!parse_hexbyte(value, byte)) {
                throw std::invalid_argument("invalid byte");
            }
            result = scan(byte);
        } else {
            log_error(ErrorType::USAGE, "Unknown scan type '" + type + "'.");
            return;
        }
    } catch (const std::bad_alloc &) {
        log_error(ErrorType::OTHER, "Not enough memory to complete the scan.");
        return;
    } catch (const std::exception &) {
        log_error(ErrorType::USAGE, "Invalid value '" + value + "' for type " + type + ".");
        return;
    }

    if (show_time) {
        log_message("Scan time: " + std::to_string(scan_time_ms) + " ms");
    }
    if (show_bytes) {
        log_message("Bytes searched: " + std::to_string(bytes_searched));
    }

    if (result == ScanResult::GPU_FAIL) {
        log_error(ErrorType::OTHER, "GPU scan failed; no store was saved. Retry with cpu.");
        return;
    }
    if (result != ScanResult::SUCCESS) {
        log_error(ErrorType::OTHER, "Memory scan failed with error code: "
            + std::to_string(static_cast<int>(result)));
        return;
    }

    size_t match_count = stored.objects.size();
    console.scan_stores.emplace(name, std::move(stored));
    log_success("Scan complete. Stored " + std::to_string(match_count)
        + " matches in '" + name + "'.");
}

void handle_refine_command(debugger_console &console, const std::vector<std::string> &args)
{
    if (args.size() < 3 || args.size() > 4) {
        log_error(ErrorType::USAGE,
            "refine <store-name> <same|changed|increased|decreased|new_value> [value]");
        return;
    }

    auto found = console.scan_stores.find(args[1]);
    if (found == console.scan_stores.end()) {
        log_error(ErrorType::OTHER, "No scan store named '" + args[1] + "'.");
        return;
    }

    FilterType filter;
    try {
        if (args[2] == "same") filter =  FilterType::SAME;
        if (args[2] == "changed") filter =  FilterType::CHANGED;
        if (args[2] == "increased") filter =  FilterType::INCREASED;
        if (args[2] == "decreased") filter =  FilterType::DECREASED;
        if (args[2] == "new_value") filter =  FilterType::NEW_VALUE;
        throw std::invalid_argument("invalid filter");
    } catch (const std::invalid_argument &) {
        log_error(ErrorType::USAGE, "Unknown refinement filter '" + args[2] + "'.");
        return;
    }

    const std::string *value = args.size() == 4 ? &args[3] : nullptr;
    if (filter == FilterType::NEW_VALUE && value == nullptr) {
        log_error(ErrorType::USAGE, "new_value requires a value.");
        return;
    }
    if (filter != FilterType::NEW_VALUE && value != nullptr) {
        log_error(ErrorType::USAGE, "Only new_value accepts a value.");
        return;
    }

    StoredScan &stored = found->second;
    MemoryObjectStore refined = MemoryObjectStore(stored.objects.get_name());
    ScanResult result;

    try {
        if (stored.scan_type == "int") {
            int filter_value = value == nullptr ? 0 : std::stoi(*value);
            result = scan_proc_memory_for_value_filtered<int>(
                console.task, stored.objects, refined, filter, filter_value
            );
        } else if (stored.scan_type == "float") {
            float filter_value = value == nullptr ? 0.0f : std::stof(*value);
            result = scan_proc_memory_for_value_filtered<float>(
                console.task, stored.objects, refined, filter, filter_value
            );
        } else if (stored.scan_type == "double") {
            double filter_value = value == nullptr ? 0.0 : std::stod(*value);
            result = scan_proc_memory_for_value_filtered<double>(
                console.task, stored.objects, refined, filter, filter_value
            );
        } else {
            unsigned char byte{};
            if (filter == FilterType::NEW_VALUE) {
                bool parsed = stored.scan_type == "rawbyte"
                    ? parse_rawbyte(*value, byte)
                    : parse_hexbyte(*value, byte);
                if (!parsed) {
                    throw std::invalid_argument("invalid byte");
                }
            }
            result = scan_proc_memory_for_value_filtered<unsigned char>(
                console.task, stored.objects, refined, filter, byte
            );
        }
    } catch (const std::exception &) {
        log_error(ErrorType::USAGE, "Invalid refinement value for type " + stored.scan_type + ".");
        return;
    }

    if (result != ScanResult::SUCCESS) {
        log_error(ErrorType::OTHER, "Memory refinement failed with error code: "
            + std::to_string(static_cast<int>(result)));
        return;
    }

    stored.objects = std::move(refined);
    log_success("Refinement complete. Store '" + args[1] + "' now contains "
        + std::to_string(stored.objects.size()) + " matches.");
}

void handle_list_command(const debugger_console &console, const std::vector<std::string> &args)
{
    if (args.size() != 2) {
        log_error(ErrorType::USAGE, "list <store-name>");
        return;
    }

    auto found = console.scan_stores.find(args[1]);
    if (found == console.scan_stores.end()) {
        log_error(ErrorType::OTHER, "No scan store named '" + args[1] + "'.");
        return;
    }

    const MemoryObjectStore &store = found->second.objects;
    std::cout << "Store " << store.get_name() << " contains "
              << store.size() << " addresses:" << std::endl;

    for (size_t i = 0; i < store.size() && i < 100; ++i) {
        std::printf("  0x%llx\n", store.at(i).address);
    }

    if (store.size() > 100) {
        std::cout << "  ... and " << (store.size() - 100) << " more" << std::endl;
    }
}

void handle_stores_command(const debugger_console &console)
{
    if (console.scan_stores.empty()) {
        log_message("No scan stores.");
        return;
    }

    for (const auto &entry : console.scan_stores) {
        std::cout << entry.first << " (" << entry.second.scan_type << "): "
                  << entry.second.objects.size() << " matches" << std::endl;
    }
}
