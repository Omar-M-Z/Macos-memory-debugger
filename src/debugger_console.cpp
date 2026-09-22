#include <iostream>
#include <vector>
#include <sstream>

#include "console_system.h"
#include "scan_commands.h"
#include "util.h"
#include "metal_resources.h"

std::vector<std::string> parse_raw_cmd(std::string input) {
    std::vector<std::string> args;
    std::istringstream stream(input);
    std::string token;
    while (stream >> token) {
        args.push_back(token);
    }
    return args;
}

debugger_console::debugger_console(int pid_arg, mach_port_t task_arg, mach_vm_address_t base_address_arg) {
    pid = pid_arg;
    task = task_arg;
    base_address = base_address_arg;

    // Set up the reusable Metal device, compute pipeline, and command queue.
    metal_resources = init_metal_resources();
}

void debugger_console::handle_command(const std::vector<std::string> &args) {
    // switching consoles and/or calling functions specific to each command implementation
    if (args[0] == "scan") {
        handle_scan_command(*this, args);
    } else if (args[0] == "refine") {
        handle_refine_command(*this, args);
    } else if (args[0] == "list") {
        handle_list_command(*this, args);
    } else if (args[0] == "stores") {
        handle_stores_command(*this);
    } else if (args[0] == "memmap") {
        log_message("memmap command not yet implemented");
    }
    else if (args[0] == "help") {
        print_help();
    }
    else {
        log_message("Command " + args[0] + " not recognized");
    }
}

int debugger_console::run() {
    std::string input;
    std::cout << prompt << " > ";
    std::getline(std::cin, input);


    std::vector<std::string> cmd = parse_raw_cmd(input);
    if (cmd.empty()) {
        return CONSOLE_OK;
    }
    if (cmd.at(0) == "quit") {
        return CONSOLE_QUIT;
    }

    handle_command(cmd);

    return CONSOLE_OK;
}

void debugger_console::print_help() {
    std::cout << "Commands:" << std::endl;
    std::cout << "  scan <store-name> -v <value> -t <int|float|double|rawbyte|hexbyte> [-gpu] [-time] [-bytes]" << std::endl;
    std::cout << "  refine <store-name> <same|changed|increased|decreased|new_value> [value]" << std::endl;
    std::cout << "  list <store-name>" << std::endl;
    std::cout << "  stores" << std::endl;
    std::cout << "  memmap" << std::endl;
    std::cout << "  quit" << std::endl;
}
