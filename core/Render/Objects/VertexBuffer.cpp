//
// Created by alexa on 2022-04-16.
//

#include "VertexBuffer.h"

#include "../Factory/FactoryVulkan.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Application.h"

VertexBuffer::~VertexBuffer() {
    // NOTE : This destructor does not obey the rule of 3 : The destructor frees resources not created by the constructor.
    // DO NOT use this class in container like the vector. Reallocation of the buffer will destroy GPU resources still in use
    VulkanRenderDevice* vrd = Application::getApp()->getRenderer()->getRenderDevice();
    vkFreeMemory(vrd->device, _deviceMemory, nullptr);
    vkDestroyBuffer(vrd->device, _buffer, nullptr);
}

void VertexBuffer::init(VulkanRenderDevice* vrd, void* data, uint32_t size) {
    // create buffer and memory
    std::tie(_buffer, _deviceMemory) = Factory::createBuffer(vrd->device, vrd->physicalDevice, size,
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // copy vertex data to it
    VK_ASSERT(utils::copyToDeviceLocalBuffer(vrd, _buffer, data, size), "Failed to copy data");
}

void VertexBuffer::bind(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_buffer, &offset);
}

std::vector<VkVertexInputAttributeDescription>
VertexBuffer::inputAttributeDescriptions(const std::vector<VertexAttribute>& attributes) {
    // create output using the size of given attributes vector
    std::vector<VkVertexInputAttributeDescription> output(attributes.size());

    // set all attributes, with incrementing location
    for (uint32_t i = 0; i < attributes.size(); ++i){
        output[i] = {
                .location = i,
                .binding = 0,
                .format = attributes[i].format,
                .offset = attributes[i].offset
        };
    }
    return output;
}

