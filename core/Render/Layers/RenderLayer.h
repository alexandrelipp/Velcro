//
// Created by alexa on 2022-03-20.
//

#pragma once


#include <vulkan/vulkan_core.h>

class RenderLayer {
public:

    virtual ~RenderLayer();
    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) = 0;

protected:
    explicit RenderLayer();


public:
    //static inline std::array<VkFramebuffer, FB_COUNT> _framebuffers{nullptr};
    static inline VkDevice _device = nullptr;
    static inline VkExtent2D _swapchainExtent{};
    //static inline DepthTexture;

    // TODO : make this global!
    static constexpr uint32_t FB_COUNT = 3;


    VkDescriptorSetLayout _descriptorSetLayout = nullptr;
    VkDescriptorPool _descriptorPool = nullptr;
    std::array<VkDescriptorSet, FB_COUNT> _descriptorSets = {nullptr};

    VkPipelineLayout _pipelineLayout_ = nullptr;
    VkPipeline _graphicsPipeline_ = nullptr;
};



