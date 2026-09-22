#pragma once

#include <string>
#include <vector>

struct debugger_console;

void handle_scan_command(debugger_console &console, const std::vector<std::string> &args);
void handle_refine_command(debugger_console &console, const std::vector<std::string> &args);
void handle_list_command(const debugger_console &console, const std::vector<std::string> &args);
void handle_stores_command(const debugger_console &console);
