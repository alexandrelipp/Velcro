//
// Created by alexa on 2022-03-14.
//

#include "ShaderStorageBuffer.h"

#include "../../Utils/UtilsVulkan.h"
#include "../Factory/FactoryVulkan.h"


void ShaderStorageBuffer::init(VulkanRenderDevice* vrd, bool hostVisible,
                               uint32_t size, void* data, VkBufferUsageFlags additionalUsage) {
    VkMemoryPropertyFlags memFlags = hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    _size = size;
    std::tie(_buffer, _bufferMemory) = Factory::createBuffer(vrd->device, vrd->physicalDevice, _size,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | additionalUsage,
                          memFlags);

    // set data if present
    if (data != nullptr)
        VK_ASSERT(setData(vrd, data, size), "Failed to set SSBO data at init");
}

void ShaderStorageBuffer::destroy(VkDevice device) {
    vkFreeMemory(device, _bufferMemory, nullptr);
    vkDestroyBuffer(device, _buffer, nullptr);

    _bufferMemory = nullptr;
    _buffer = nullptr;
}

uint32_t ShaderStorageBuffer::getSize() const {
    return _size;
}

VkBuffer ShaderStorageBuffer::getBuffer() const {
    return _buffer;
}

//////////////////// HOST Shader storage buffer object /////////////////////////
void HostSSBO::init(VulkanRenderDevice* vrd, uint32_t size, void* data, VkBufferUsageFlags additionalUsage) {
    ShaderStorageBuffer::init(vrd, true, size, data, additionalUsage);
}

bool HostSSBO::setData(VulkanRenderDevice* vrd, void* data, uint32_t size) {
    if (size > _size)
        return false;

    // if the buffer is host visible, simply copy memory (no need for a staging buffer)
    void* dst = nullptr;
    VK_CHECK(vkMapMemory(vrd->device, _bufferMemory, 0, size, 0, &dst));
    memcpy(dst, data, size);
    vkUnmapMemory(vrd->device, _bufferMemory);
    return true;
}

//////////////////// DEVICE Shader storage buffer object /////////////////////////
void DeviceSSBO::init(VulkanRenderDevice* vrd, uint32_t size, void* data, VkBufferUsageFlags additionalUsage) {
    ShaderStorageBuffer::init(vrd, false, size, data, additionalUsage);
}

bool DeviceSSBO::setData(VulkanRenderDevice* vrd, void* data, uint32_t size) {
    if (size > _size)
        return false;

    // use a staging buffer to copy to device local buffer
    return utils::copyToDeviceLocalBuffer(vrd, _buffer, data, size);
}
