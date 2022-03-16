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

   /// sync objects
   VkSemaphore createSemaphore(VkDevice device);
   VkFence createFence(VkDevice device, bool startSignaled); /// signaled = available to used

   VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

   VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D& extent, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const ShaderFiles& shaders);

   /// memory
   std::pair<VkBuffer, VkDeviceMemory> createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
       VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

   std::pair<VkImage, VkDeviceMemory> createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                                                  uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling
                                                  VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

   /// descriptors
   VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t imageCount,
                                                          uint32_t uniformBufferCount,
                                                          uint32_t storageBufferCount,
                                                          uint32_t samplerCount);
}



