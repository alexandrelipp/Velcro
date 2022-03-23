//
// Created by alexa on 2022-03-02.
//

#pragma once

#include "VulkanRenderDevice.hpp"
#include "Objects/UniformBuffer.h"
#include "Objects/ShaderStorageBuffer.h"
#include "Objects/Texture.h"
#include "Layers/RenderLayer.h"

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

    // 
    void recordCommandBuffer(uint32_t index);

private:
    static constexpr uint32_t FB_COUNT = 3; ///< triple buffering is used

private:
    // context
    VulkanRenderDevice _vrd{};    ///< vulkan render device, immutable after initialization

    // swapchain
    VkSwapchainKHR _swapchain = nullptr;
    std::array<VkImage, FB_COUNT> _swapchainImages = {nullptr};
    std::array<VkImageView, FB_COUNT> _swapchainImageViews = {nullptr};
    VkExtent2D _swapchainExtent = {0, 0};

    // other
    VkRenderPass _renderPass = nullptr;
    VkSurfaceKHR _surface = nullptr;
    std::array<VkFramebuffer, FB_COUNT> _frameBuffers = {nullptr};

    // commands
    glm::vec4 _clearValue = { 0.3f, 0.5f, 0.5f, 1.f };

    // sync
    //VkFence _inFlightFence = nullptr;
    VkSemaphore _imageAvailSemaphore = nullptr;
    VkSemaphore _renderFinishedSemaphore = nullptr;

    // DepthBuffer
    struct DepthBuffer{
        VkImage image = nullptr;
        VkImageView imageView = nullptr;
        VkDeviceMemory deviceMemory = nullptr;
        VkFormat format;
    } _depthBuffer;

    // Render layers
    std::vector<std::shared_ptr<RenderLayer>> _renderLayers;

    /// ONLY PRESENT IN DEBUG ///
#ifdef VELCRO_DEBUG
    VkDebugUtilsMessengerEXT _messenger = nullptr;
    VkDebugReportCallbackEXT _reportCallback = nullptr;
#endif
};



