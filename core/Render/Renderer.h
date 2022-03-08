//
// Created by alexa on 2022-03-02.
//

#pragma once

#include <vulkan/vulkan.h>


class Renderer {
public:
    Renderer();
    ~Renderer();
    bool init();

private:
    void createInstance();

private:
    static constexpr uint32_t FB_COUNT = 3; ///< triple buffering is used

private:
    // context
    VkInstance _instance = nullptr;
    VkPhysicalDevice _physicalDevice = nullptr;
    VkDevice _device = nullptr; ///< Logical device
    uint32_t _graphicsQueueFamilyIndex; ///< index of the graphics family

    VkSurfaceKHR _surface = nullptr;

    // swapchain
    VkSwapchainKHR _swapchain = nullptr;
    std::array<VkImage, FB_COUNT> _swapchainImages;
    std::array<VkImageView, FB_COUNT> _swapchainImageViews;

    // commands
    VkCommandPool _commandPool = nullptr;
    std::array<VkCommandBuffer, FB_COUNT> _commandBuffers;

    /// ONLY PRESENT IN DEBUG ///
#ifdef VELCRO_DEBUG
    VkDebugUtilsMessengerEXT _messenger = nullptr;
    VkDebugReportCallbackEXT _reportCallback = nullptr;
#endif
};



