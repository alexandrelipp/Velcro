//
// Created by alexa on 2022-03-04.
//

#pragma once

#include "../VulkanRenderDevice.hpp"

#include <vulkan/vulkan.h>
#include <optional>
#include <string>
#include <variant>

struct ShaderFiles {
    std::optional<std::string> vertex = std::nullopt;
    VkSpecializationInfo* vertexSpec = nullptr;
    std::optional<std::string> fragment = std::nullopt;
    VkSpecializationInfo* fragmentSpec = nullptr;
    std::optional<std::string> geometry = std::nullopt;
};


namespace Factory {
    // callbacks
   bool setupDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);
   void freeDebugCallbacks(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkDebugReportCallbackEXT reportCallback);

   /// create a logical device
   VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex,
                         const VkPhysicalDeviceFeatures& features);

   /// sync objects
   VkSemaphore createSemaphore(VkDevice device);
   VkFence createFence(VkDevice device, bool startSignaled); /// signaled = available to used

   VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

   struct GraphicsPipelineProps{
       VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
       ShaderFiles shaders;
       VkBool32 enableDepthTest = VK_TRUE;
       VkBool32 enableStencilTest = VK_FALSE;
       std::optional<VkStencilOpState> frontStencilState = std::nullopt;
       VkBool32 enableBlending = VK_TRUE;
       VkBool32 enableBackFaceCulling = VK_TRUE;
       VkSampleCountFlagBits sampleCountMSAA = VK_SAMPLE_COUNT_1_BIT;
       std::vector<VkDynamicState> dynamicStates;
   };
   VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D& extent, VkRenderPass renderPass,
                                     VkPipelineLayout pipelineLayout, const GraphicsPipelineProps& props);

   /// memory
   std::pair<VkBuffer, VkDeviceMemory> createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
       VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

   std::pair<VkImage, VkDeviceMemory> createImage(VulkanRenderDevice* vrd, VkSampleCountFlagBits sampleCount,
                                                  uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                                  VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

   VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

   /// descriptors
   VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t imageCount,
                                                          uint32_t uniformBufferCount,
                                                          uint32_t storageBufferCount,
                                                          uint32_t samplerImageCount);

   /// describes a descriptor. For now the following are supported :
   /// - Array of textures
   /// - One descriptor per frame in flight (can be duplicated if ressource is the same for both frame in flight)
   struct Descriptor {
       VkDescriptorType type;
       VkShaderStageFlags shaderStage;
       std::variant<std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>, std::vector<VkDescriptorImageInfo>> info;
   };
   std::tuple<VkDescriptorSetLayout, VkPipelineLayout, VkDescriptorPool, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>>
           createDescriptorSets(VulkanRenderDevice* renderDevice, const std::vector<Descriptor>& descriptors,
                                const std::vector<VkPushConstantRange>& pushConstants);
}

