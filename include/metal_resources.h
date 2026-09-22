#pragma once

#include <optional>

#include <Metal/Metal.hpp>

struct MetalResources {
    MTL::Device *device;
    MTL::Library *library;
    MTL::Function *scan_function;
    MTL::ComputePipelineState *scan_pipeline_state;
    MTL::CommandQueue *command_queue;
};

std::optional<MetalResources> init_metal_resources();
