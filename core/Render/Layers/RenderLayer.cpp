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
