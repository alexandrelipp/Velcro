//
// Created by alexa on 2022-04-11.
//

#include "SelectedMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Utils/UtilsTemplate.h"

SelectedMeshLayer::SelectedMeshLayer(VkRenderPass renderPass) {
    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    // init vertices ssbo
    std::vector<TexVertex2> vertices;
    VK_ASSERT(FactoryModel::createTexturedSquare2(vertices), "Failed to gen vertices");

    uint32_t verticesSize = utils::vectorSizeByte(vertices);
    _vertices.init(_vrd->device, _vrd->physicalDevice, verticesSize);
    _vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool, vertices.data(),
                      verticesSize);

    // describe descriptors
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_mvpUniformBuffers[0].getBuffer(), 0, _mvpUniformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_mvpUniformBuffers[1].getBuffer(), 0, _mvpUniformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                    }
            },
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {});

    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "SelectedMeshV.spv",
                    .fragment = "SelectedMeshF.spv"
            },
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);

}

SelectedMeshLayer::~SelectedMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
}

void SelectedMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    glm::mat4 data(1.f);
    _mvpUniformBuffers[commandBufferIndex].setData(_vrd->device, &data, sizeof(data));
}

void SelectedMeshLayer::onEvent(Event& event) {

}

void SelectedMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    bindPipelineAndDS(commandBuffer, commandBufferIndex);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdDraw(commandBuffer, 6, 1, 0, 1);

    //vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffff);
    //vkCmdSetStencilOp(commandBuffer, )
}

void SelectedMeshLayer::onImGuiRender() {

}

