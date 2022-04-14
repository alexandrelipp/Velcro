//
// Created by alexa on 2022-04-11.
//

#include "SelectedMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Utils/UtilsTemplate.h"

SelectedMeshLayer::SelectedMeshLayer(VkRenderPass renderPass, std::shared_ptr<Scene> scene, const ShaderStorageBuffer& vertices,
                                     const ShaderStorageBuffer& indices) : _scene(scene) {
    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(SelectedMeshMVP));

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
                            VkDescriptorBufferInfo {vertices.getBuffer(), 0, vertices.getSize()},
                            VkDescriptorBufferInfo {vertices.getBuffer(), 0, vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {indices.getBuffer(), 0, indices.getSize()},
                            VkDescriptorBufferInfo {indices.getBuffer(), 0, indices.getSize()},
                    }
            },
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {});

    // Fill up front stencil state as described by the spec :
    // https://www.khronos.org/registry/vulkan/specs/1.3/html/chap26.html#fragops-stencil
    VkStencilOpState frontStencilState = {
            .failOp = VK_STENCIL_OP_KEEP,      // will be set dynamically
            .passOp = VK_STENCIL_OP_KEEP,      // will be set dynamically
            .depthFailOp = VK_STENCIL_OP_KEEP, // operation if stencil passes, but depth fails. Will never happen, depth test is disabled
            .compareOp = VK_COMPARE_OP_NEVER,  // will be set dynamically
            .compareMask = 0xffffffff,         // compare mask
            .writeMask = 0xffffffff,           // always write
            .reference = 0,
    };
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "SelectedMeshV.spv",
                    .fragment = "SelectedMeshF.spv"
            },
            .enableDepthTest = VK_FALSE, // depth dest is disabled
            .enableStencilTest = VK_TRUE,
            .frontStencilState = frontStencilState,
            .sampleCountMSAA = _vrd->sampleCount,
            .dynamicStates = {
                    VK_DYNAMIC_STATE_STENCIL_OP, // we dynamically change the stencil operation
                    }
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);

}

SelectedMeshLayer::~SelectedMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_vrd->device);
}

void SelectedMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_selectedMesh == nullptr || _selectedEntity == -1)
        return;
    glm::mat4 model = _scene->getTransform(_selectedEntity).worldTransform;
    SelectedMeshMVP mvps = {
            .original = pv * model,
            .scaledUp = pv * glm::scale(model, glm::vec3(1.1f))
    };
    _mvpUniformBuffers[commandBufferIndex].setData(_vrd->device, &mvps, sizeof(mvps));
}

void SelectedMeshLayer::onEvent(Event& event) {}

void SelectedMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    if (_selectedMesh == nullptr)
        return;
    bindPipelineAndDS(commandBuffer, commandBufferIndex);

    // at the beginning of the render pass, the stencil buffer is cleared with 0's

    //
    vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_INCREMENT_AND_CLAMP, VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                      VK_STENCIL_OP_KEEP, VK_COMPARE_OP_GREATER);

    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffffff);
    vkCmdDraw(commandBuffer, _selectedMesh->indexCount, 1, _selectedMesh->firstVertexIndex, 0);

    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0);

    vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
                      VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
    vkCmdDraw(commandBuffer, _selectedMesh->indexCount, 1, _selectedMesh->firstVertexIndex, 1);

    //vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffff);

}

void SelectedMeshLayer::onImGuiRender() {

}

void SelectedMeshLayer::setSelectedEntity(int selectedEntity) {
    _selectedEntity = selectedEntity;
    _selectedMesh = _scene->getMesh(_selectedEntity);
    if (_selectedMesh == nullptr) {
        SPDLOG_INFO("Failed to find a mesh component for entity {}", _selectedEntity);
        return;
    }
    SPDLOG_INFO("Selected mesh name {}",_scene->getName(selectedEntity));

}

