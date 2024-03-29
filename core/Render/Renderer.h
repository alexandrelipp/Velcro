//
// Created by alexa on 2022-03-02.
//

#pragma once

#include "VulkanRenderDevice.hpp"
#include "Objects/UniformBuffer.h"
#include "Objects/ShaderStorageBuffer.h"
#include "Objects/Texture.h"
#include "Layers/RenderLayer.h"
#include "Layers/ImGuiLayer.h"
#include "../events/Event.h"
#include "Camera/Camera.h"
#include "FPSCounter.hpp"

#include <vulkan/vulkan.h>


class Renderer {
public:
    Renderer(float initialAspectRatio);
    ~Renderer();
    bool init();

    VulkanRenderDevice* getRenderDevice();
    VkExtent2D getSwapchainExtent();

    void draw(float dt);
    void onEvent(Event& e);

    Camera* getCamera();
private:

    // creation
    void createInstance();
    void createSwapchain(const VkSurfaceFormatKHR& surfaceFormat);
    void createRenderPass(VkFormat swapchainFormat);

    // 
    void recordCommandBuffer(uint32_t commandBufferIndex, VkFramebuffer framebuffer);

    void onImGuiRender();


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
    VkFence _renderFinishedFence = nullptr; ///< signaled when the GPU is done rendering a frame
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailSpres = {nullptr};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSpres = {nullptr};
    bool _currentFiFIndex = true; ///< Index of the current frame in flight being recorded on CPU

    /// AttachmentBuffer, used by multisampled color buffer and depth/stencil buffer
    struct AttachmentBuffer{
        VkImage image = nullptr;
        VkImageView imageView = nullptr;
        VkDeviceMemory deviceMemory = nullptr;
        VkFormat format;
    };
    AttachmentBuffer _depthBuffer;
    AttachmentBuffer _colorBuffer;

    // Render layers
    std::vector<std::shared_ptr<RenderLayer>> _renderLayers;
    std::shared_ptr<ImGuiLayer> _imGuiLayer = nullptr;
    bool _imguiFocus = false;

    // camera
    Camera _camera;

    FPSCounter _fpsCounter;

    /// ONLY PRESENT IN DEBUG ///
#ifdef VELCRO_DEBUG
    VkDebugUtilsMessengerEXT _messenger = nullptr;
    VkDebugReportCallbackEXT _reportCallback = nullptr;
#endif
};



