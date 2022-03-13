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

   // sync objects
   VkSemaphore createSemaphore(VkDevice device);

   /// signaled = available to used
   VkFence createFence(VkDevice device, bool startSignaled);

   VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

   VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D& extent, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const ShaderFiles& shaders);

   std::pair<VkBuffer, VkDeviceMemory> createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
       VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
}



