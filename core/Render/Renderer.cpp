//
// Created by alexa on 2022-03-02.
//

#include "Renderer.h"
#include "../../Application.h"

#include "../Utils/UtilsVulkan.h"
#include "../Utils/UtilsFile.h"
#include "Factory/FactoryVulkan.h"
#include "Factory/FactoryModel.h"
#include "Layers/ModelLayer.h"


Renderer::Renderer() {

}

Renderer::~Renderer() {
    VK_CHECK(vkDeviceWaitIdle(_vrd.device));

    // destroy sync objects
    //vkDestroyFence(_vrd.device, _inFlightFence, nullptr);
    vkDestroySemaphore(_vrd.device, _imageAvailSemaphore, nullptr);
    vkDestroySemaphore(_vrd.device, _renderFinishedSemaphore, nullptr);

    for (auto fb : _frameBuffers)
        vkDestroyFramebuffer(_vrd.device, fb, nullptr);
    vkDestroyRenderPass(_vrd.device, _renderPass, nullptr);

    _renderLayers.clear();

    vkFreeCommandBuffers(_vrd.device, _vrd.commandPool, FB_COUNT, _vrd.commandBuffers.data());
    vkDestroyCommandPool(_vrd.device, _vrd.commandPool, nullptr);
    for (auto view : _swapchainImageViews) {
        vkDestroyImageView(_vrd.device, view, nullptr);
    }

    // free depth buffer
    vkDestroyImageView(_vrd.device, _depthBuffer.imageView, nullptr);
    vkDestroyImage(_vrd.device, _depthBuffer.image, nullptr);
    vkFreeMemory(_vrd.device, _depthBuffer.deviceMemory, nullptr);

    vkDestroySwapchainKHR(_vrd.device, _swapchain, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_vrd.device, nullptr);
#ifdef VELCRO_DEBUG
    Factory::freeDebugCallbacks(_instance, _messenger, _reportCallback);
#endif
    vkDestroyInstance(_instance, nullptr);
}

bool Renderer::init() {
    // create the context
    createInstance();

    // pick a physical device (gpu)
    _vrd.physicalDevice = utils::pickPhysicalDevice(_instance);
    utils::printPhysicalDeviceProps(_vrd.physicalDevice);

    // create a logical device (interface to gpu)
    utils::printQueueFamiliesInfo(_vrd.physicalDevice);
    _vrd.graphicsQueueFamilyIndex = utils::getQueueFamilyIndex(_vrd.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
    _vrd.device = Factory::createDevice(_vrd.physicalDevice, _vrd.graphicsQueueFamilyIndex);

    // create surface
    VK_CHECK(glfwCreateWindowSurface(_instance, Application::getApp()->getWindow(), nullptr, &_surface));

    // make sure the graphics queue supports presentation
    VkBool32 presentationSupport;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_vrd.physicalDevice, _vrd.graphicsQueueFamilyIndex, _surface, &presentationSupport));
    VK_ASSERT(presentationSupport == VK_TRUE, "Graphics queue does not support presentation");

    // retreive queue handle
    vkGetDeviceQueue(_vrd.device, _vrd.graphicsQueueFamilyIndex, 0, &_vrd.graphicsQueue);

    // pick a format and a present mode for the surface
    VkSurfaceFormatKHR surfaceFormat = utils::pickSurfaceFormat(_vrd.physicalDevice, _surface);

    // create the swapchain
    createSwapchain(surfaceFormat);
    VK_ASSERT(_swapchain != nullptr, "Failed to create swapchain");

    // retreive images from the swapchain after making sure the count is correct. Note : images are freed automatically when the SP is destroyed
    uint32_t count;
    VK_CHECK(vkGetSwapchainImagesKHR(_vrd.device, _swapchain, &count, nullptr));
    VK_ASSERT(count == FB_COUNT, "images count in swapchain does not match FB count");
    VK_CHECK(vkGetSwapchainImagesKHR(_vrd.device, _swapchain, &count, _swapchainImages.data()));

    // create image views from the fetched images
    VkImageAspectFlags flags = VK_IMAGE_ASPECT_COLOR_BIT;
    for (int i = 0; i < FB_COUNT; ++i) {
        _swapchainImageViews[i] = Factory::createImageView(_vrd.device, _swapchainImages[i], surfaceFormat.format, flags);
    }

    _depthBuffer.format = utils::findSupportedFormat(_vrd.physicalDevice,
                                                     {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                     VK_IMAGE_TILING_OPTIMAL,
                                                     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    auto depth = Factory::createImage(_vrd.device, _vrd.physicalDevice, _swapchainExtent.width, _swapchainExtent.height,
                                      _depthBuffer.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _depthBuffer.image = depth.first;
    _depthBuffer.deviceMemory = depth.second;

    _depthBuffer.imageView = Factory::createImageView(_vrd.device, depth.first, _depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT);

    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = _vrd.graphicsQueueFamilyIndex
    };
    VK_CHECK(vkCreateCommandPool(_vrd.device, &commandPoolCreateInfo, nullptr, &_vrd.commandPool));

    // allocate command buffers from created command pool
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = _vrd.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = FB_COUNT,
    };
    VK_CHECK(vkAllocateCommandBuffers(_vrd.device, &allocateInfo, _vrd.commandBuffers.data()));
   
    createRenderPass(surfaceFormat.format);

    _renderLayers.push_back(std::make_shared<ModelLayer>(_renderPass));

    // create framebuffers
    for (int i = 0; i < FB_COUNT; ++i) {
        std::array<VkImageView, 2> attachments = {_swapchainImageViews[i], _depthBuffer.imageView};
        VkFramebufferCreateInfo framebufferCI = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .flags = 0u,
        .renderPass = _renderPass,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .width = _swapchainExtent.width,
        .height = _swapchainExtent.height,
        .layers = 1,
        };
        VK_CHECK(vkCreateFramebuffer(_vrd.device, &framebufferCI, nullptr, &_frameBuffers[i]));
    }

    // create sync objects
    //_inFlightFence = Factory::createFence(_vrd.device, true); // starts signaled
    _imageAvailSemaphore = Factory::createSemaphore(_vrd.device);
    _renderFinishedSemaphore = Factory::createSemaphore(_vrd.device);
  
    return true;
}

VulkanRenderDevice* Renderer::getRenderDevice() {
    return &_vrd;
}

VkExtent2D Renderer::getSwapchainExtent() {
    return _swapchainExtent;
}


void Renderer::draw() {
    // wait until the fence is signaled (ready to use)
    //VK_CHECK(vkWaitForFences(_vrd.device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX));

    // then unsignal the fence for next use
    //VK_CHECK(vkResetFences(_vrd.device, 1, &_inFlightFence));


    uint32_t imageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_vrd.device, _swapchain, UINT64_MAX, _imageAvailSemaphore, nullptr, &imageIndex));

    for (auto layer : _renderLayers)
        layer->update(0.001f, imageIndex);

    //VK_CHECK(vkResetCommandBuffer(_vrd.commandBuffers[imageIndex], 0));
    // reset the command pool, probably not optimal but good enough for now
    VK_CHECK(vkResetCommandPool(_vrd.device, _vrd.commandPool, 0));
    recordCommandBuffer(imageIndex);

    // semaphore check to occur before writing to the color attachment
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_imageAvailSemaphore, // wait until signaled before starting
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &_vrd.commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &_renderFinishedSemaphore // signaled when command buffer is done executing
    };
    VK_CHECK(vkQueueSubmit(_vrd.graphicsQueue, 1, &submitInfo, nullptr));


    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    VK_CHECK(vkQueuePresentKHR(_vrd.graphicsQueue, &presentInfo));

    // wait for completion of all operation on graphics queue (not optimal, but good enough for now)
    VK_CHECK(vkDeviceWaitIdle(_vrd.device));
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
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_vrd.physicalDevice, _surface, &capabilites));

    // get the swap chain extent. The FB size is required if surface capabilites does not contain valid value (unlikely)
    int FBwidth, FBheight;
    glfwGetFramebufferSize(Application::getApp()->getWindow(), &FBwidth, &FBheight);
    _swapchainExtent = utils::pickSwapchainExtent(capabilites, FBwidth, FBheight);

    // we request to have at least one more FB then the min image count, to prevent waiting on GPU
    SPDLOG_INFO("Min image count with current GPU and surface {}", capabilites.minImageCount);
    VK_ASSERT(capabilites.minImageCount < FB_COUNT, "Number wrong");

    VkPresentModeKHR presentMode = utils::pickSurfacePresentMode(_vrd.physicalDevice, _surface);

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
            .queueFamilyIndexCount = 0u,                            // only relevant when sharing mode is Concurrent
            .pQueueFamilyIndices = nullptr,                         // only relevant when sharing mode is Concurrent
            .preTransform = capabilites.currentTransform,           // Transform applied to image before presentation
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,    // blending mode with other surfaces
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
    };
    VK_CHECK(vkCreateSwapchainKHR(_vrd.device, &createInfo, nullptr, &_swapchain));
}

void Renderer::createRenderPass(VkFormat swapchainFormat){
    std::array<VkAttachmentDescription, 2> attachments{};
    attachments[0] = {
      .flags = 0u,
      .format = swapchainFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT, // not using multisampling
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // operation on color and depth at beginning of subpass
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, //  Rendered contents will be stored in memory and can be read later
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // we don't use stencil
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,  // we don't use stencil
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout of the image subressource when subpass begin. We don't care ; we clear it anyway
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // layout to be transitioned automatically when render pass instance ends
                                                              //We want the image to be ready for presentation using the swap chain after rendering

    };

    VkAttachmentReference colorRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    attachments[1] = {
        .flags = 0u,
        .format = _depthBuffer.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthRef = {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    // a render pass can consist of multiple subpasses. In our case we only use 1
    VkSubpassDescription subpassDescription = {
        .flags = 0u,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0u,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,  // The index of the attachment in this array is directly referenced from 
                                         //the fragment shader with the layout(location = 0) out vec4 outColor directive!
        .pDepthStencilAttachment = &depthRef
    };

    std::vector<VkSubpassDependency> dependencies = {
        /* VkSubpassDependency */ {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0
        }
    };

    // create our render pass with one attachment and one subpass
    VkRenderPassCreateInfo renderPassCI = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .flags = 0u,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = (uint32_t)dependencies.size(),
        .pDependencies = dependencies.data()
    };

    VK_CHECK(vkCreateRenderPass(_vrd.device, &renderPassCI, nullptr, &_renderPass));
}

void Renderer::recordCommandBuffer(uint32_t index){
    
    // begin recording command
    VkCommandBufferBeginInfo commandBufferCI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK(vkBeginCommandBuffer(_vrd.commandBuffers[index], &commandBufferCI));

    // being render pass
    VkRect2D renderArea = {
        .offset = {
            .x = 0,
            .y = 0
        },
        .extent = _swapchainExtent
    };

    // TODO : is there a way to type pun this?
    VkClearValue clearValues[] = {
            {
                .color = {_clearValue.r, _clearValue.g, _clearValue.b, _clearValue.a}
            },
            {
                .depthStencil = {.depth = 1.f}
            }
    };
    VkRenderPassBeginInfo beginCI = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = _renderPass,
        .framebuffer = _frameBuffers[index],
        .renderArea = renderArea,
        .clearValueCount = sizeof(clearValues)/sizeof(VkClearValue),
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(_vrd.commandBuffers[index], &beginCI, VK_SUBPASS_CONTENTS_INLINE);

    // record render commands from all the layers
    for (auto layer : _renderLayers)
        layer->fillCommandBuffer(_vrd.commandBuffers[index], index);

    // end the render pass
    vkCmdEndRenderPass(_vrd.commandBuffers[index]);

    // stop recording commands
    VK_CHECK(vkEndCommandBuffer(_vrd.commandBuffers[index]));
}