#include <iostream>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>

#include "console_system.h"
#include "util.h"

// getting base address of a process in its address space
int get_aslr_base_address(mach_port_t task, mach_vm_address_t *address_out)
{
    // getting dyld info from task port
    struct task_dyld_info dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t ret = task_info((task_name_t)task, TASK_DYLD_INFO, (task_info_t)&dyld_info, &count);
    if (ret != KERN_SUCCESS)
        return 1;

    // getting info about the images in the address space
    struct dyld_all_image_infos all_infos;
    mach_vm_size_t size = sizeof(all_infos);
    ret = mach_vm_read_overwrite(task, dyld_info.all_image_info_addr, size, (mach_vm_address_t)&all_infos, &size);
    if (ret != KERN_SUCCESS)
        return 1;

    // getting the base address using the images
    struct dyld_image_info first_image;
    mach_vm_size_t image_info_size = sizeof(first_image);
    ret = mach_vm_read_overwrite(task, (mach_vm_address_t)all_infos.infoArray, image_info_size, (mach_vm_address_t)&first_image, &image_info_size);
    if (ret != KERN_SUCCESS)
        return 1;
    *address_out = (mach_vm_address_t)first_image.imageLoadAddress;

    return 0;
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        log_error(ErrorType::USAGE, "Please provide a PID to attach to.");
        return 1;
    }

    int pid = std::stoi(argv[1]);

    // getting the task port of the given pid
    mach_port_t task;
    kern_return_t ret = task_for_pid(mach_task_self(), pid, &task);
    if (ret != KERN_SUCCESS)
    {
        log_error(ErrorType::OTHER, "Failed to get task port ID.");
        return 1;
    }
    log_message("Attached to PID: " + std::to_string(pid));
    log_message("Task port ID: " + std::to_string(task));

    // getting base address
    mach_vm_address_t base_address;
    if (get_aslr_base_address(task, &base_address) != 0)
    {
        log_error(ErrorType::OTHER, "Failed to get process base address pointer.");
        mach_port_deallocate(mach_task_self(), task);
        return 1;
    }

    // building the debugger console
    debugger_console dc = debugger_console(pid, task, base_address);

    // running the console
    while (true) {
        int ret = dc.run();
        if (ret == CONSOLE_ERROR){
            log_error(ErrorType::OTHER, "Error running debugger");
            break;
        }
        if (ret == CONSOLE_QUIT){
            log_message("Quitting . . .");
            break;
        }
    }

    /*
    std::vector<mach_vm_address_t> matching_addresses;
    scan_proc_memory_for_value(task, 123456, matching_addresses);

    // printing addresses
    for (mach_vm_address_t addr : matching_addresses)
    {
        std::cout << "0x" << std::hex << addr << std::endl;
    }*/

    mach_port_deallocate(mach_task_self(), task);
    return 0;
}
