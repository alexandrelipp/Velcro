//
// Created by alexa on 2022-04-16.
//

#pragma once

#include <vulkan/vulkan_core.h>
#include "../VulkanRenderDevice.hpp"


class VertexBuffer {
public:
    VertexBuffer() = default;
    ~VertexBuffer();

    void init(VulkanRenderDevice* vrd, void* data, uint32_t size);

private:
    VkBuffer _buffer = nullptr;
    VkDeviceMemory _deviceMemory = nullptr;
};
