//
// Created by alexa on 2022-03-04.
//

#pragma once

#include <vulkan/vulkan.h>

namespace utils {
    bool isInstanceExtensionSupported(const char* extension);
    bool isInstanceLayerSupported(const char* layer);

    // Device
    VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
    void printPhysicalDeviceProps(VkPhysicalDevice device);
    bool isDeviceExtensionSupported(VkPhysicalDevice device, const char* extension);

    // Queue
    uint32_t getQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueFlags);
    void printQueueFamiliesInfo(VkPhysicalDevice device);

    // Surface
    VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkPresentModeKHR pickSurfacePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilites, int frameBufferW, int frameBufferH);

    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    // Memory
    int findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}