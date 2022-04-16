//
// Created by alexa on 2022-04-16.
//

#include "VertexBuffer.h"

#include "../Factory/FactoryVulkan.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Application.h"

VertexBuffer::~VertexBuffer() {
    VulkanRenderDevice* vrd = Application::getApp()->getRenderer()->getRenderDevice();
    vkFreeMemory(vrd->device, _deviceMemory, nullptr);
    vkDestroyBuffer(vrd->device, _buffer, nullptr);
}

void VertexBuffer::init(VulkanRenderDevice* vrd, void* data, uint32_t size) {
    // create buffer and memory
    std::tie(_buffer, _deviceMemory) = Factory::createBuffer(vrd->device, vrd->physicalDevice, size,
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // copy vertex data to it
    VK_ASSERT(utils::copyToDeviceLocalBuffer(vrd, _buffer, data, size), "Failed to copy data");
}
