//
// Created by alexa on 2022-04-17.
//

#include "IndexBuffer.h"

#include "../Factory/FactoryVulkan.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Application.h"

// Potential optimization : might be more memory friendly to store vertices and indices in one contiguous buffer,
// as described in https://developer.nvidia.com/vulkan-memory-management

IndexBuffer::~IndexBuffer() {
    // NOTE : This destructor does not obey the rule of 3 : The destructor frees resources not created by the constructor.
    // DO NOT use this class in container like the vector. Reallocation of the buffer will destroy GPU resources still in use
    VulkanRenderDevice* vrd = Application::getApp()->getRenderer()->getRenderDevice();
    vkFreeMemory(vrd->device, _deviceMemory, nullptr);
    vkDestroyBuffer(vrd->device, _buffer, nullptr);
}

void IndexBuffer::init(VulkanRenderDevice* vrd, VkIndexType indexType, void* data, uint32_t indexCount) {
    _indexType = indexType;
    _indexCount = indexCount;

    // calculate size of the index buffer in bytes
    uint32_t size = _indexCount * indexTypeSize(indexType);

    // create buffer and memory
    std::tie(_buffer, _deviceMemory) = Factory::createBuffer(vrd->device, vrd->physicalDevice, size,
                                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // copy vertex data to it
    VK_ASSERT(utils::copyToDeviceLocalBuffer(vrd, _buffer, data, size), "Failed to copy data");
}

void IndexBuffer::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindIndexBuffer(commandBuffer, _buffer, 0, _indexType);
}

uint32_t IndexBuffer::getIndexCount() const {
    return _indexCount;
}

/// Returns number of bytes for given index type
constexpr uint32_t IndexBuffer::indexTypeSize(VkIndexType type) {
    switch (type) {
        case VK_INDEX_TYPE_UINT8_EXT:
            return sizeof(uint8_t);
        case VK_INDEX_TYPE_UINT16:
            return sizeof(uint16_t);
        case VK_INDEX_TYPE_UINT32:
            return sizeof(uint32_t);
        default:
            VK_ASSERT(false, "Invalid Index data type");
    }
    return 0;
}