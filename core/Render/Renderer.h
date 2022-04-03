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
    Renderer();
    ~Renderer();
    bool init();

    VulkanRenderDevice* getRenderDevice();
    VkExtent2D getSwapchainExtent();

    void update(float dt);
    void draw();
    void onEvent(Event& e);

    Camera* getCamera();
private:

    // creation
    void createInstance();
    void createSwapchain(const VkSurfaceFormatKHR& surfaceFormat);
    void createRenderPass(VkFormat swapchainFormat);

    // 
    void recordCommandBuffer(uint32_t index);

    void onImGuiRender();

private:
    static constexpr uint32_t FB_COUNT = 3;                         ///< triple buffering is used
    ///< max number of frames processed by cpu or gpu. This way the recording of a frame (cpu) does not have to wait for the gpu to finish rendering.
    /// Then, when the GPU is done rendering, it does not need to wait for the cpu to record command. This means ressources like uniform buffer will be duplicated
    /// because can't be uploaded (while recording the next frame) and used (while rendering the current frame) at the same time
    // https://www.reddit.com/r/vulkan/comments/nbu94q/what_exactly_is_the_definition_of_frames_in_flight/
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = FB_COUNT - 1;


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



