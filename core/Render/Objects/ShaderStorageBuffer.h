//
// Created by alexa on 2022-03-14.
//

#pragma once

#include <vulkan/vulkan.h>


class ShaderStorageBuffer {
    ShaderStorageBuffer() = default;

    void init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size);

    bool setData(VkDevice device, VkPhysicalDevice physicalDevice,
                 VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size);

private:
    VkBuffer _buffer = nullptr;
    VkDeviceMemory _bufferMemory = nullptr;
    uint32_t _size = 0;

};