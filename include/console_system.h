#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include "metal_resources.h"
#include "util.h"

const int CONSOLE_ERROR = 1;
const int CONSOLE_OK = 0;
const int CONSOLE_QUIT = -1;

struct StoredScan {
    MemoryObjectStore objects;
    std::string scan_type;
};

struct debugger_console
{
    debugger_console(int pid, mach_port_t task_arg, mach_vm_address_t base_address);

    std::string prompt = "debugger";

    int pid;
    mach_port_t task;
    mach_vm_address_t base_address;

    std::unordered_map<std::string, StoredScan> scan_stores;
    std::optional<MetalResources> metal_resources = std::nullopt;

    int run();
    void handle_command(const std::vector<std::string> &args);
    void print_help();
};
