//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "../VulkanRenderDevice.hpp"
#include "../../events/Event.h"
#include "../Factory/FactoryVulkan.h"
#include "../../Scene/Scene.h"

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>


class RenderLayer {
public:

    virtual ~RenderLayer();

    virtual void update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) = 0;
    virtual void onEvent(Event& event) = 0;

    // TODO : it is a bit redundant to pass both the command buffer and the index or we don't care?
    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) = 0;
    virtual void onImGuiRender() = 0;

    static std::shared_ptr<Scene> getCurrentScene();

protected:
    explicit RenderLayer();

    // Reusable helper methods for render layers

    /// Binds the graphics pipeline and the descriptor set at the given command buffer index
    void bindPipelineAndDS(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex);


protected:
    static inline VulkanRenderDevice* _vrd = nullptr;
    static inline VkExtent2D _swapchainExtent{};

    // descriptors
    VkDescriptorSetLayout _descriptorSetLayout = nullptr;
    VkDescriptorPool _descriptorPool = nullptr;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets = {nullptr};

    // graphics pipeline
    VkPipelineLayout _pipelineLayout = nullptr;
    VkPipeline _graphicsPipeline = nullptr;

private:
    static inline std::shared_ptr<Scene> _currentScene = nullptr;
};



