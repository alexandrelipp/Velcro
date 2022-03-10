//
// Created by alexa on 2022-03-02.
//

#include "Renderer.h"
#include "../../Application.h"

#include "../Utils/UtilsVulkan.h"
#include "../Utils/UtilsFile.h"
#include "Factory/FactoryVulkan.h"


Renderer::Renderer() {

}

Renderer::~Renderer() {
    vkFreeCommandBuffers(_device, _commandPool, FB_COUNT, _commandBuffers.data());
    vkDestroyCommandPool(_device, _commandPool, nullptr);
    for (auto view : _swapchainImageViews) {
        vkDestroyImageView(_device, view, nullptr);
    }
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
#ifdef VELCRO_DEBUG
    Factory::freeDebugCallbacks(_instance, _messenger, _reportCallback);
#endif
    vkDestroyInstance(_instance, nullptr);
}

bool Renderer::init() {
    // create the context
    createInstance();

    // pick a physical device (gpu)
    _physicalDevice = utils::pickPhysicalDevice(_instance);
    utils::printPhysicalDeviceProps(_physicalDevice);

    // create a logical device (interface to gpu)
    utils::printQueueFamiliesInfo(_physicalDevice);
    _graphicsQueueFamilyIndex = utils::getQueueFamilyIndex(_physicalDevice, VK_QUEUE_GRAPHICS_BIT);
    _device = Factory::createDevice(_physicalDevice, _graphicsQueueFamilyIndex);

    // create surface
    VK_CHECK(glfwCreateWindowSurface(_instance, Application::getApp()->getWindow(), nullptr, &_surface));

    // make sure the graphics queue supports presentation
    VkBool32 presentationSupport;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, _graphicsQueueFamilyIndex, _surface, &presentationSupport));
    VK_ASSERT(presentationSupport == VK_TRUE, "Graphics queue does not support presentation");

    // pick a format and a present mode for the surface
    VkSurfaceFormatKHR surfaceFormat = utils::pickSurfaceFormat(_physicalDevice, _surface);

    // create the swapchain
    createSwapchain(surfaceFormat);
    VK_ASSERT(_swapchain != nullptr, "Failed to create swapchain");

    // retreive images from the swapchain after making sure the count is correct. Note : images are freed automatically when the SP is destroyed
    uint32_t count;
    VK_CHECK(vkGetSwapchainImagesKHR(_device, _swapchain, &count, nullptr));
    VK_ASSERT(count == FB_COUNT, "images count in swapchain does not match FB count");
    VK_CHECK(vkGetSwapchainImagesKHR(_device, _swapchain, &count, _swapchainImages.data()));

    // create image views from the fetched images
    VkImageAspectFlags flags = VK_IMAGE_ASPECT_COLOR_BIT;
    for (int i = 0; i < FB_COUNT; ++i) {
        _swapchainImageViews[i] = utils::createImageView(_device, _swapchainImages[i], surfaceFormat.format, flags);
    }

    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .queueFamilyIndex = _graphicsQueueFamilyIndex
    };
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandPool));

    // allocate command buffers from created command pool
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = _commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = FB_COUNT,
    };
    VK_CHECK(vkAllocateCommandBuffers(_device, &allocateInfo, _commandBuffers.data()));

    ShaderFiles files = {
        .vertex = "vert.spv",
        .fragment = "frag.spv"
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_device, _swapchainExtent, _renderPass, )
  
    return true;
}

void Renderer::createInstance() {
    std::vector<const char*> extensions;
    uint32_t count;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    for (int i = 0; i < count; ++i){
        extensions.push_back(glfwExtensions[i]);
    }

#ifdef VELCRO_DEBUG
    VK_ASSERT(utils::isInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME), "Extension not supported");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    for (auto e : extensions){
        VK_ASSERT(utils::isInstanceExtensionSupported(e), "Extension not supported");
        SPDLOG_INFO("Enabled instanced extension {}", e);
    }

    // NOTE : we could enable more layers, fe VK_LAYER_NV_GPU_Trace_release_public_2021_5_1 or VK_LAYER_RENDERDOC_Capture
    std::vector<const char*> layers;
#ifdef VELCRO_DEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    for (auto layer : layers){
        VK_ASSERT(utils::isInstanceLayerSupported(layer), "Layer not supported");
        SPDLOG_INFO("Enabled layer {}", layer);
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "HelloVelcro",
            .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
            .pEngineName = "Velcro",
            .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
            .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = (uint32_t)layers.size(),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = (uint32_t)extensions.size(),
            .ppEnabledExtensionNames = extensions.data(),
    };
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &_instance));

#ifdef VELCRO_DEBUG
    Factory::setupDebugCallbacks(_instance, &_messenger, &_reportCallback);
#endif
}

void Renderer::createSwapchain(const VkSurfaceFormatKHR& surfaceFormat){
    VkSurfaceCapabilitiesKHR capabilites;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilites));

    // get the swap chain extent. The FB size is required if surface capabilites does not contain valid value (unlikely)
    int FBwidth, FBheight;
    glfwGetFramebufferSize(Application::getApp()->getWindow(), &FBwidth, &FBheight);
    _swapchainExtent = utils::pickSwapchainExtent(capabilites, FBwidth, FBheight);

    // we request to have at least one more FB then the min image count, to prevent waiting on GPU
    SPDLOG_INFO("Min image count with current GPU and surface {}", capabilites.minImageCount);
    VK_ASSERT(capabilites.minImageCount < FB_COUNT, "Number wrong");

    VkPresentModeKHR presentMode = utils::pickSurfacePresentMode(_physicalDevice, _surface);

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0u,
            .surface = _surface,
            .minImageCount = FB_COUNT,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = _swapchainExtent,
            .imageArrayLayers = 1,                                  // number of views in a multiview/stereo (holo) surface. Always 1 either
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |         // image can be used as the destination of a transfer command
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,      // image can be used to create a view for use as a color attachment // TODO : necesary?
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,          // swapchain is not shared between queue (only used by graphics queue)
            .queueFamilyIndexCount = 0u,                            // only relevant when sharing mode is Concurent
            .pQueueFamilyIndices = nullptr,                         // only relevant when sharing mode is Concurent
            .preTransform = capabilites.currentTransform,           // Transform applied to image before presentation
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,    // blending mode with other surfaces
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
    };
    VK_CHECK(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain));
}
