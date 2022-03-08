//
// Created by alexa on 2022-03-04.
//

#pragma once

#include <vulkan/vulkan.h>


namespace Factory {
    // callbacks
   bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);
   void freeDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkDebugReportCallbackEXT reportCallback);

   /// create a logical device
   VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex);

   VkSwapchainKHR createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
       VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, uint32_t framebufferCount);

   VkSemaphore createSemaphore(VkDevice device);
}



