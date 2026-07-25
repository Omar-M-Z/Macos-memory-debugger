#include <iostream>
#include <unistd.h>

int main() {
    int secret_value = 123456;
    
    // Print the Process ID so you know who to attach to
    std::cout << "Target PID: " << getpid() << std::endl;
    
    // Print the exact memory address of our secret value
    std::cout << "Target Address: " << &secret_value << std::endl;
    std::cout << "Value to find: " << secret_value << std::endl;

    std::cout << "\nWaiting for Analyst to read... (Press Ctrl+C to stop)" << std::endl;
    
    while (true) {
        std::cout << "still alive" << std::endl;
        sleep(10); // Stay alive so we can read the memory
    }
    
    return 0;
}