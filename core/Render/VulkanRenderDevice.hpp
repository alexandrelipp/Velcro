#pragma once

#include <vulkan/vulkan_core.h>
#include <array>

static constexpr uint32_t FB_COUNT = 3;         ///< triple buffering is used

///< max number of frames processed by cpu or gpu. This way the recording of a frame (cpu) does not have to wait for the gpu to finish rendering.
/// Then, when the GPU is done rendering, it does not need to wait for the cpu to record command. This means ressources like uniform buffer will be duplicated
/// because can't be uploaded (while recording the next frame) and used (while rendering the current frame) at the same time
// https://www.reddit.com/r/vulkan/comments/nbu94q/what_exactly_is_the_definition_of_frames_in_flight/
static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = FB_COUNT - 1;

/// All members in this struct are immutable after creation
struct VulkanRenderDevice final{
    // context
    VkInstance instance = nullptr;
    VkDevice device = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;

    // queue
    uint32_t graphicsQueueFamilyIndex = 0; ///< index of the graphics family
    VkQueue graphicsQueue = nullptr;

    // commands
    VkCommandPool commandPool = nullptr;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers = {nullptr};

    // pipeline
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
};