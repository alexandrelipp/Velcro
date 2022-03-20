//
// Created by alexa on 2022-03-02.
//

#pragma once

#include "VulkanRenderDevice.hpp"
#include "Objects/UniformBuffer.h"
#include "Objects/ShaderStorageBuffer.h"
#include "Objects/Texture.h"

#include <vulkan/vulkan.h>


class Renderer {
public:
    Renderer();
    ~Renderer();
    bool init();

    VulkanRenderDevice* getRenderDevice();
    VkExtent2D getSwapchainExtent();

    void draw();
private:

    // creation
    void createInstance();
    void createSwapchain(const VkSurfaceFormatKHR& surfaceFormat);
    void createRenderPass(VkFormat swapchainFormat);
    void createPipelineLayout();
    void createDescriptorSets();

    // 
    void recordCommandBuffer(uint32_t index);

private:
    static constexpr uint32_t FB_COUNT = 3; ///< triple buffering is used

private:
    // context
    VkInstance _instance = nullptr;
    VulkanRenderDevice _vrd{};    ///< vulkan render device, immutable after initialization

    VkSurfaceKHR _surface = nullptr;

    // swapchain
    VkSwapchainKHR _swapchain = nullptr;
    std::array<VkImage, FB_COUNT> _swapchainImages = {nullptr};
    std::array<VkImageView, FB_COUNT> _swapchainImageViews = {nullptr};
    VkExtent2D _swapchainExtent = {0, 0};


    // pipeline
    VkPipeline _graphicsPipeline = nullptr;
    VkRenderPass _renderPass = nullptr;
    VkPipelineLayout _pipelineLayout = nullptr;

    std::array<VkFramebuffer, FB_COUNT> _frameBuffers = {nullptr};

    // commands
    glm::vec4 _clearValue = { 0.3f, 0.5f, 0.5f, 1.f };

    // sync
    //VkFence _inFlightFence = nullptr;
    VkSemaphore _imageAvailSemaphore = nullptr;
    VkSemaphore _renderFinishedSemaphore = nullptr;

    // Buffers
    std::array<UniformBuffer, FB_COUNT> _mvpUniformBuffers{};
    glm::mat4 mvp = glm::mat4(1.f);
    ShaderStorageBuffer _vertices{};
    ShaderStorageBuffer _indices{};
    uint32_t _indexCount = 0;

    // Descriptors
    VkDescriptorPool _descriptorPool = nullptr;
    VkDescriptorSetLayout _descriptorSetLayout = nullptr;
    std::array<VkDescriptorSet, FB_COUNT> _descriptorSets{nullptr};

    // Textures
    Texture _texture{};

    // DepthBuffer
    struct DepthBuffer{
        VkImage image = nullptr;
        VkImageView imageView = nullptr;
        VkDeviceMemory deviceMemory = nullptr;
        VkFormat format;
    } _depthBuffer;

    /// ONLY PRESENT IN DEBUG ///
#ifdef VELCRO_DEBUG
    VkDebugUtilsMessengerEXT _messenger = nullptr;
    VkDebugReportCallbackEXT _reportCallback = nullptr;
#endif
};



