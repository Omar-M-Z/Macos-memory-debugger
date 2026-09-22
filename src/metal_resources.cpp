#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// metal-cpp is header-only. These macros make exactly this source file provide
// its Foundation and Metal wrapper implementations. They need to be defined
// before any metal-cpp header is included, and in only one source file.
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>

#include "metal_resources.h"

std::optional<MetalResources> init_metal_resources()
{
    NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

    // creating metal device
    MTL::Device *device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "Metal is unavailable on this Mac.\n";
        pool->release();
        return std::nullopt;
    }

    // Read and compile src/scan_memory.metal at runtime. This does not require full Xcode's
    // offline `metal` compiler or a default.metallib file.
    std::ifstream metal_file("src/scan_memory.metal");
    if (!metal_file) {
        std::cerr << "Could not open src/scan_memory.metal. Run this from the project root.\n";
        device->release();
        pool->release();
        return std::nullopt;
    }

    std::ostringstream metal_source_stream;
    metal_source_stream << metal_file.rdbuf();
    const std::string metal_source = metal_source_stream.str();

    NS::Error *error = nullptr;
    MTL::Library *library = device->newLibrary(
        NS::String::string(metal_source.c_str(), NS::UTF8StringEncoding),
        nullptr,
        &error);
    if (!library) {
        std::cerr << "Could not compile src/scan_memory.metal: "
                  << (error ? error->localizedDescription()->utf8String() : "unknown error")
                  << '\n';
        device->release();
        pool->release();
        return std::nullopt;
    }

    MTL::Function *scan_function = library->newFunction(MTLSTR("scan_memory_for_value"));
    if (!scan_function) {
        std::cerr << "No kernel named 'scan_memory_for_value' was found in src/scan_memory.metal.\n";
        library->release();
        device->release();
        pool->release();
        return std::nullopt;
    }

    // creating compute pipeline state
    error = nullptr;
    MTL::ComputePipelineState *scan_pipeline_state = device->newComputePipelineState(scan_function, &error);
    if (!scan_pipeline_state) {
        std::cerr << "Could not create compute pipeline: "
                  << (error ? error->localizedDescription()->utf8String() : "unknown error")
                  << '\n';
        scan_function->release();
        library->release();
        device->release();
        pool->release();
        return std::nullopt;
    }

    // command queue will be initialized once and shared while command buffer and compute encoder will be created for each task
    MTL::CommandQueue *command_queue = device->newCommandQueue();
    if (!command_queue) {
        std::cerr << "Could not create Metal command queue.\n";
        scan_pipeline_state->release();
        scan_function->release();
        library->release();
        device->release();
        pool->release();
        return std::nullopt;
    }

    MetalResources metal_resources;
    metal_resources.device = device;
    metal_resources.library = library;
    metal_resources.scan_function = scan_function;
    metal_resources.scan_pipeline_state = scan_pipeline_state;
    metal_resources.command_queue = command_queue;

    pool->release();

    return std::make_optional(metal_resources);
}
