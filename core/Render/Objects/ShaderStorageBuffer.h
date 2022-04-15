//
// Created by alexa on 2022-03-14.
//

#pragma once

#include <vulkan/vulkan.h>


class ShaderStorageBuffer {
public:
    ShaderStorageBuffer() = default;

    // TODO : should just take a VRD, and should have a init setting data!
    void init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size,
              bool hostVisible = false, VkBufferUsageFlags additionalUsage = 0);
    void destroy(VkDevice device);

    uint32_t getSize() const;
    VkBuffer getBuffer() const;

    // TODO : should just take a VRD
    bool setData(VkDevice device, VkPhysicalDevice physicalDevice,
                 VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size);

private:
    VkBuffer _buffer = nullptr;
    VkDeviceMemory _bufferMemory = nullptr;
    uint32_t _size = 0;

    // TODO : should be split in two, stupid
    bool _hostVisible = false;
};