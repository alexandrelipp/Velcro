//
// Created by alexa on 2022-03-04.
//

#pragma once

#include <vulkan/vulkan.h>

struct ShaderFiles {
    std::optional<std::string> vertex = std::nullopt;
    std::optional<std::string> geometry = std::nullopt;
    std::optional<std::string> fragment = std::nullopt;
};


namespace Factory {
    // callbacks
   bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);
   void freeDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkDebugReportCallbackEXT reportCallback);

   /// create a logical device
   VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex);

   VkSemaphore createSemaphore(VkDevice device);

   VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

   VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D& extent, VkRenderPass renderPass, const ShaderFiles& shaders);
}



