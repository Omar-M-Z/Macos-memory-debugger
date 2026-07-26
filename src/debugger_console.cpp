#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include "console_system.h"
#include "scan_console.h"
#include "util.h"

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
}

void debugger_console::handle_command(const std::vector<std::string> &args) {
    // switching consoles and/or calling functions specific to each command implementation
    if (args[0] == "scan"){
        try {
            this->active_sub_console = std::make_unique<scan_console>(*this, args);
        } catch (const std::invalid_argument&) {}
    } 
    else if (args[0] == "help") {
        log_message("help command not yet implemented");
    }
    else {
        log_message("Command " + args[0] + " not recognized");
    }
}

int debugger_console::run() {
    std::string input;
    if (active_sub_console == nullptr) { // use main console (there is no active subconsole)
        std::cout << prompt << " > ";
    } else { // sub console is active and command should be pushed to it
        std::cout << prompt << " > " << active_sub_console->prompt << " > ";
    }
    std::getline(std::cin, input);


    std::vector<std::string> cmd = parse_raw_cmd(input);
    if (cmd.at(0) == "quit") {
        return CONSOLE_QUIT;
    }

    if (active_sub_console == nullptr){
        handle_command(cmd);
    } else {
        active_sub_console->handle_command(cmd);
    }

    return 0;
}

void debugger_console::remove_active_sub_console() {
    this->active_sub_console.reset();
}
