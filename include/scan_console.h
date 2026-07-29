#pragma once
#include "console_system.h"
#include "util.h"

struct scan_console : sub_console {
    
    /**
    * Scans a process's memory for a target value of a given type
    * Iterates over readable memory regions and searches for
    * occurrences of the specified value, adding them to the output vector.
    *
    * @param parent the parent console of this scan console
    * @param args arg0 is the value to scan for, arg1 is the type (int/float/double/byte)
    *
    * @return 0 on success, non-zero on failure.
    */
    scan_console(debugger_console &parent, const std::vector<std::string> &args);

    /**
     * Returns the prompt for the scan console.
     * @return the prompt string
     */
    std::string get_prompt() const override;
    
    /**
     * Handles commands entered in the scan console. This is where the scanning logic will be triggered based on user input.
     * @param args the command arguments entered by the user in the scan console 
     * @return void
     */
    void handle_command(const std::vector<std::string> &args) override;
    ~scan_console();

private:
    void print_available_commands() const;
    void print_scan_results() const;
    void print_help() const;

    MemoryObjectStore scan_results;
    std::string scan_type;  // "int", "float", "double", or "byte"
    std::string target_value_str;
    const std::string prompt = "memscan";
};
