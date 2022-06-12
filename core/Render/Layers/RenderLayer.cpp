//
// Created by alexa on 2022-03-20.
//

#include "RenderLayer.h"

#include "../../Application.h"

RenderLayer::RenderLayer() {
    // init static members if not already done
    if (_vrd == nullptr || _vrd->device == nullptr){
        Renderer* renderer = Application::getApp()->getRenderer();
        _vrd = renderer->getRenderDevice();
        _swapchainExtent = renderer->getSwapchainExtent();
        _currentScene = std::make_shared<Scene>("NanoWorld");
    }
}

RenderLayer::~RenderLayer() {
    // destroy descriptors
    if (_descriptorSetLayout != nullptr)
        vkDestroyDescriptorSetLayout(_vrd->device, _descriptorSetLayout, nullptr);
    if (_descriptorPool != nullptr)
        vkDestroyDescriptorPool(_vrd->device, _descriptorPool, nullptr);

    // destroy pipeline
    if (_pipelineLayout != nullptr)
        vkDestroyPipelineLayout(_vrd->device, _pipelineLayout, nullptr);
    if (_graphicsPipeline != nullptr)
        vkDestroyPipeline(_vrd->device, _graphicsPipeline, nullptr);
}

/// Binds the graphics pipeline and the descriptor set at the given command buffer index
void RenderLayer::bindPipelineAndDS(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[commandBufferIndex], 0, nullptr);
}

std::shared_ptr<Scene> RenderLayer::getCurrentScene() {
    return _currentScene;
}