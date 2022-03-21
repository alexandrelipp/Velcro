//
// Created by alexa on 2022-03-20.
//

#include "LineLayer.h"
#include "../Factory/FactoryVulkan.h"

LineLayer::LineLayer(VkRenderPass renderPass) : RenderLayer() {
    float aspectRatio = _swapchainExtent.width/(float)_swapchainExtent.height;
    glm::mat4 v = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 p = glm::perspective(45.f, aspectRatio, 0.1f, 1000.f);
    glm::mat4 mvp = p * v;

    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers) {
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));
        buffer.setData(_vrd->device, glm::value_ptr(mvp), sizeof(mvp));
    }

    std::vector<VertexData> stubData = {
            VertexData{},
            VertexData{
                .position = glm::vec3(1.f),
                .color = glm::vec4(1.f, 0.1f, 0.2f, 1.f)
            }
    };

    auto size = stubData.size() * sizeof(VertexData);
    _pointsSSBO.init(_vrd->device, _vrd->physicalDevice, size);
    _pointsSSBO.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool, stubData.data(), size);

    createPipelineLayout();
    createDescriptorSets();

    Factory::GraphicsPipelineProps props = {
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
            .shaders = {
                    .vertex = "lineV.spv",
                    .fragment = "lineF.spv"
            }
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

LineLayer::~LineLayer() {

}

void LineLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0,
                            1, &_descriptorSets[currentImage], 0, nullptr);

    vkCmdDraw()
}

void LineLayer::update(float dt, uint32_t currentImage) {

}

void LineLayer::createPipelineLayout() {

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
            },
    };

    VkDescriptorSetLayoutCreateInfo layoutCI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = bindings.size(),
            .pBindings = bindings.data(),
    };

    VK_CHECK(vkCreateDescriptorSetLayout(_vrd->device, &layoutCI, nullptr, &_descriptorSetLayout));

    VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags = 0u,
        .setLayoutCount = 1,
        .pSetLayouts = &_descriptorSetLayout
    };

    VK_CHECK(vkCreatePipelineLayout(_vrd->device, &layoutCreateInfo, nullptr, &_pipelineLayout));
}

void LineLayer::createDescriptorSets() {
    _descriptorPool = Factory::createDescriptorPool(_vrd->device, FB_COUNT, 1, 1, 0);

    std::array<VkDescriptorSetLayout, FB_COUNT> layouts = {_descriptorSetLayout, _descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_vrd->device, &descriptorSetAI, _descriptorSets.data()));

    for (size_t i = 0; i < _descriptorSets.size(); ++i) {
        // create write descriptor set for each descriptor set
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        // push back descriptor write for UBO (1 buffer/image)
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _mvpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _mvpUniformBuffers[i].getSize()
        };
        writeDescriptorSets.push_back({
                                      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                      .dstSet = _descriptorSets[i],
                                      .dstBinding = 0,
                                      .dstArrayElement = 0,
                                      .descriptorCount = 1,
                                      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      .pBufferInfo = &bufferInfo
                                      });

        // push back descriptor write for vertices SSBO (1 buffer)
        VkDescriptorBufferInfo verticesInfo = {
                .buffer = _pointsSSBO.getBuffer(),
                .offset = 0,
                .range = _pointsSSBO.getSize()
        };
        writeDescriptorSets.push_back({
                                      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                      .dstSet = _descriptorSets[i],
                                      .dstBinding = 1,
                                      .dstArrayElement = 0,
                                      .descriptorCount = 1,
                                      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                      .pBufferInfo = &verticesInfo
                                      });

        // update the descriptor sets with the created decriptor writes
        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}
