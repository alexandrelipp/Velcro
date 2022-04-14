//
// Created by alexa on 2022-04-11.
//

#include "SelectedMeshLayer.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Utils/UtilsTemplate.h"

// Better way to do this using post processing : http://geoffprewett.com/blog/software/opengl-outline/

SelectedMeshLayer::SelectedMeshLayer(VkRenderPass renderPass, const Props& props) : _scene(props.scene) {
    // init the uniform buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    // describe descriptors
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vpUniformBuffers[0].getBuffer(), 0, _vpUniformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {_vpUniformBuffers[1].getBuffer(), 0, _vpUniformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {props.vertices.getBuffer(), 0, props.vertices.getSize()},
                            VkDescriptorBufferInfo {props.vertices.getBuffer(), 0, props.vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {props.indices.getBuffer(), 0, props.indices.getSize()},
                            VkDescriptorBufferInfo {props.indices.getBuffer(), 0, props.indices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {props.meshTransformBuffers[0].getBuffer(), 0, props.meshTransformBuffers[0].getSize()},
                            VkDescriptorBufferInfo {props.meshTransformBuffers[1].getBuffer(), 0, props.meshTransformBuffers[1].getSize()},
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
    Factory::GraphicsPipelineProps factoryProps = {
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
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, factoryProps);

}

SelectedMeshLayer::~SelectedMeshLayer() {
    // destroy the buffers
    for (auto& buffer : _vpUniformBuffers)
        buffer.destroy(_vrd->device);
}

void SelectedMeshLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (_selectedEntity == -1 || _selectedMeshes.empty() )
        return;
//    glm::mat4 model = _scene->getTransform(_selectedEntity).worldTransform;
//    SelectedMeshMVP mvps = {
//            .original = pv * model,
//            .scaledUp = pv * glm::scale(model, glm::vec3(MAG_STENCIL_FACTOR))
//    };
    _vpUniformBuffers[commandBufferIndex].setData(_vrd->device, &pv, sizeof(pv));
}

void SelectedMeshLayer::onEvent(Event& event) {}

void SelectedMeshLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    // nothing to do if no selected entity
    if (_selectedEntity == -1 || _selectedMeshes.empty() )
        return;

    // bind the layer
    bindPipelineAndDS(commandBuffer, commandBufferIndex);

    // at the beginning of the render pass, the stencil buffer is cleared with 0's

    // TODO : maybe don't change stencil op every time ??
    for (auto mesh : _selectedMeshes){
        //
        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_INCREMENT_AND_CLAMP, VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_GREATER);
        //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffffff);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, 0);

        //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0);

        vkCmdSetStencilOp(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
                          VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL);
        vkCmdDraw(commandBuffer, mesh->indexCount, 1, mesh->firstVertexIndex, 1);
    }



    //vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
    //vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_BIT, 0xffffff);

}

void SelectedMeshLayer::onImGuiRender() {

}

void SelectedMeshLayer::setSelectedEntity(int selectedEntity) {
    // set selected entity
    _selectedEntity = selectedEntity;
    _selectedMeshes.clear();

    // nothing to do if no selected entity
    if (selectedEntity == -1)
        return;

    // append all mesh components that are child of the selected entity
    _scene->traverseRecursive(_selectedEntity, [this](int entity){
        MeshComponent* mesh = _scene->getMesh(entity);
        if (mesh != nullptr)
            _selectedMeshes.push_back(mesh);
    });
    SPDLOG_INFO("Selected mesh name {}", _scene->getName(selectedEntity));
}

