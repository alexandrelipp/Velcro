#pragma once

#include <vulkan/vulkan_core.h>
#include <array>

static constexpr uint32_t FB_COUNT = 3; ///< triple buffering is used

/// All members in this struct are immutable after creation
struct VulkanRenderDevice final{
    // context
    VkDevice device = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;

    // queue
    uint32_t graphicsQueueFamilyIndex = 0; ///< index of the graphics family
    VkQueue graphicsQueue = nullptr;

    // commands
    VkCommandPool commandPool = nullptr;
    std::array<VkCommandBuffer, FB_COUNT> commandBuffers = {nullptr};

};