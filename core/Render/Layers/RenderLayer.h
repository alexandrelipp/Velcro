//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "../VulkanRenderDevice.hpp"

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>


class RenderLayer {
public:

    virtual ~RenderLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) = 0;

    // TODO : it is a bit redundant to pass both the command buffer and the index or we don't care?
    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) = 0;
    virtual void onImGuiRender() = 0;

protected:
    explicit RenderLayer();


public:
    //static inline std::array<VkFramebuffer, FB_COUNT> _framebuffers{nullptr};
    static inline VulkanRenderDevice* _vrd = nullptr;
    static inline VkExtent2D _swapchainExtent{};
    //static inline DepthTexture;


    // descriptors
    VkDescriptorSetLayout _descriptorSetLayout = nullptr;
    VkDescriptorPool _descriptorPool = nullptr;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets = {nullptr};

    // graphics pipeline
    VkPipelineLayout _pipelineLayout = nullptr;
    VkPipeline _graphicsPipeline = nullptr;
};



