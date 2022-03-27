//
// Created by alexa on 2022-03-14.
//

#include "ShaderStorageBuffer.h"

#include "../../Utils/UtilsVulkan.h"
#include "../Factory/FactoryVulkan.h"


void ShaderStorageBuffer::init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size,
                               VkBufferUsageFlags additionalUsage) {
    _size = size;
    auto [buffer, memory] = Factory::createBuffer(device, physicalDevice, _size,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | additionalUsage,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _buffer = buffer;
    _bufferMemory = memory;
}

void ShaderStorageBuffer::destroy(VkDevice device) {
    vkFreeMemory(device, _bufferMemory, nullptr);
    vkDestroyBuffer(device, _buffer, nullptr);

    _bufferMemory = nullptr;
    _buffer = nullptr;
}

uint32_t ShaderStorageBuffer::getSize() {
    return _size;
}

VkBuffer ShaderStorageBuffer::getBuffer() {
    return _buffer;
}


bool ShaderStorageBuffer::setData(VkDevice device, VkPhysicalDevice physicalDevice,
                             VkQueue queue, VkCommandPool commandPool, void* data, uint32_t size) {
    if (size > _size)
        return false;
    auto stagingBuffer = Factory::createBuffer(device, physicalDevice, size,
                                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    // copy from data -> staging buffer
    void *dst;
    VK_CHECK(vkMapMemory(device, stagingBuffer.second, 0, _size, 0u, &dst));
    memcpy(dst, data, size);
    vkUnmapMemory(device, stagingBuffer.second);


    // copy from staging buffer -> buffer
    utils::executeOnQueueSync(queue, device, commandPool, [=, this](VkCommandBuffer commandBuffer){
        VkBufferCopy bufferCopy = {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = size
        };
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.first, _buffer, 1, &bufferCopy);
    });

    // destroy staging buffer
    vkFreeMemory(device, stagingBuffer.second, nullptr);
    vkDestroyBuffer(device, stagingBuffer.first, nullptr);
    return true;
}