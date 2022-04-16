//
// Created by alexa on 2022-03-14.
//

#pragma once

#include <vulkan/vulkan.h>
#include "../VulkanRenderDevice.hpp"


class ShaderStorageBuffer {
protected:
    ShaderStorageBuffer() = default;

public:
    virtual bool setData(VulkanRenderDevice* vrd, void* data, uint32_t size) = 0;


    void destroy(VkDevice device);

    uint32_t getSize() const;
    VkBuffer getBuffer() const;

protected:
    // TODO : should just take a VRD, and should have a init setting data!
    void init(VulkanRenderDevice* vrd, bool hostVisible, uint32_t size,
              void* data = nullptr, VkBufferUsageFlags additionalUsage = 0);

protected:
    VkBuffer _buffer = nullptr;
    VkDeviceMemory _bufferMemory = nullptr;
    uint32_t _size = 0;

};

class HostSSBO : public ShaderStorageBuffer {
public:
    HostSSBO() = default;

    void init(VulkanRenderDevice* vrd, uint32_t size, void* data = nullptr, VkBufferUsageFlags additionalUsage = 0);
    virtual bool setData(VulkanRenderDevice* vrd, void* data, uint32_t size) override;
};

class DeviceSSBO : public ShaderStorageBuffer {
public:
    DeviceSSBO() = default;

    void init(VulkanRenderDevice* vrd, uint32_t size, void* data = nullptr, VkBufferUsageFlags additionalUsage = 0);
    virtual bool setData(VulkanRenderDevice* vrd, void* data, uint32_t size) override;
};