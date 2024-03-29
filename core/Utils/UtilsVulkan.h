//
// Created by alexa on 2022-03-04.
//

#pragma once

#include <vulkan/vulkan.h>
#include <functional>
#include <vector>
#include "../Render/VulkanRenderDevice.hpp"

namespace utils {
    bool isInstanceExtensionSupported(const char* extension);
    bool isInstanceLayerSupported(const char* layer);

    // Device
    VkPhysicalDevice pickPhysicalDevice(VkInstance instance, const VkPhysicalDeviceFeatures& features);
    void printPhysicalDeviceProps(VkPhysicalDevice device);
    bool isDeviceExtensionSupported(VkPhysicalDevice device, const char* extension);
    bool isFeaturesSupported(const VkPhysicalDeviceFeatures& supportedFeatures,
                             const VkPhysicalDeviceFeatures& requestedFeatures);

    // Queue
    uint32_t getQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueFlags);
    void printQueueFamiliesInfo(VkPhysicalDevice device);
    void executeOnQueueSync(VkQueue queue, VkDevice device, VkCommandPool pool,const std::function<void(VkCommandBuffer)>& commands);

    // Surface
    VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkPresentModeKHR pickSurfacePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilites, int frameBufferW, int frameBufferH);

    // images
    bool transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool pool,
                               VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    // Memory
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool copyToDeviceLocalBuffer(VulkanRenderDevice* vrd, VkBuffer buffer, void* data, uint32_t size);

    // format
    /// from the given candidates, returns the first supporting the given tiling and formatFeatures
    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
                                 VkImageTiling tiling, VkFormatFeatureFlags features);
    //VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
    bool hasStencilComponent(VkFormat format);

    /// Returns the maximum sample count (depth + color) for the given GPU
    VkSampleCountFlagBits getMaximumSampleCount(VkPhysicalDevice physicalDevice);
}