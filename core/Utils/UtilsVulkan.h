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

    // Queue
    uint32_t getGraphicsQueueFamilyIndex(VkPhysicalDevice device);
    void printQueueFamiliesInfo(VkPhysicalDevice device);
}