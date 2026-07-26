#pragma once
#include <memory>
#include <string>
#include <vector>
#include <mach/mach.h>
#include <mach/mach_vm.h>

const int CONSOLE_ERROR = 1;
const int CONSOLE_OK = 0;
const int CONSOLE_QUIT = -1;

struct debugger_console;

struct sub_console
{
    explicit sub_console(debugger_console& parent) : parent(parent) {}

    std::string prompt;
    debugger_console &parent;

    virtual void handle_command(const std::vector<std::string> &args) = 0;

    virtual ~sub_console() = default;
};

struct debugger_console
{
    debugger_console(int pid, mach_port_t task_arg, mach_vm_address_t base_address);

    std::string prompt = "debugger";

    int pid;
    mach_port_t task;
    mach_vm_address_t base_address;

    int run();
    void handle_command(const std::vector<std::string> &args);
    void print_help();

    std::unique_ptr<sub_console> active_sub_console;

    void remove_active_sub_console();
};
