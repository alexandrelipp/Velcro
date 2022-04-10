//
// Created by alexa on 2022-03-02.
//

#include "Renderer.h"
#include "../Application.h"

#include "../Utils/UtilsVulkan.h"
#include "../Utils/UtilsFile.h"
#include "Factory/FactoryVulkan.h"
#include "Factory/FactoryModel.h"
#include "Layers/ModelLayer.h"
#include "Layers/LineLayer.h"
#include "Layers/MultiMeshLayer.h"
#include "Layers/FlipbookLayer.h"

#include <imgui/imgui.h>


Renderer::Renderer() : _camera(1.f),
                       _fpsCounter(0.5f){}

Renderer::~Renderer() {
    VK_CHECK(vkDeviceWaitIdle(_vrd.device));

    // destroy sync objects
    vkDestroyFence(_vrd.device, _renderFinishedFence, nullptr);
    for (auto& semaphore : _imageAvailSpres)
        vkDestroySemaphore(_vrd.device, semaphore, nullptr);
    for (auto& semaphore : _renderFinishedSpres)
        vkDestroySemaphore(_vrd.device, semaphore, nullptr);

    for (auto fb : _frameBuffers)
        vkDestroyFramebuffer(_vrd.device, fb, nullptr);
    vkDestroyRenderPass(_vrd.device, _renderPass, nullptr);

    // clear render layer vector to trigger destructors (they should not be referenced elswhere)
    _renderLayers.clear();
    _imGuiLayer = nullptr;

    vkFreeCommandBuffers(_vrd.device, _vrd.commandPool, _vrd.commandBuffers.size(), _vrd.commandBuffers.data());
    vkDestroyCommandPool(_vrd.device, _vrd.commandPool, nullptr);
    for (auto view : _swapchainImageViews) {
        vkDestroyImageView(_vrd.device, view, nullptr);
    }

    // free depth buffer
    vkDestroyImageView(_vrd.device, _depthBuffer.imageView, nullptr);
    vkDestroyImage(_vrd.device, _depthBuffer.image, nullptr);
    vkFreeMemory(_vrd.device, _depthBuffer.deviceMemory, nullptr);

    // free color buffer
    vkDestroyImageView(_vrd.device, _colorBuffer.imageView, nullptr);
    vkDestroyImage(_vrd.device, _colorBuffer.image, nullptr);
    vkFreeMemory(_vrd.device, _colorBuffer.deviceMemory, nullptr);

    vkDestroySwapchainKHR(_vrd.device, _swapchain, nullptr);
    vkDestroySurfaceKHR(_vrd.instance, _surface, nullptr);
    vkDestroyDevice(_vrd.device, nullptr);
#ifdef VELCRO_DEBUG
    Factory::freeDebugCallbacks(_vrd.instance, _messenger, _reportCallback);
#endif
    vkDestroyInstance(_vrd.instance, nullptr);
}

bool Renderer::init() {
    OPTICK_EVENT();
    // create the context
    createInstance();

    // gpu feature to be enabled (all disabled by default)
    VkPhysicalDeviceFeatures features{};
    features.geometryShader = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    features.multiDrawIndirect = VK_TRUE;
    features.drawIndirectFirstInstance = VK_TRUE;
    //features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

    // pick a physical device (gpu)
    _vrd.physicalDevice = utils::pickPhysicalDevice(_vrd.instance, features);
    utils::printPhysicalDeviceProps(_vrd.physicalDevice);

    // create a logical device (interface to gpu)
    utils::printQueueFamiliesInfo(_vrd.physicalDevice);
    _vrd.graphicsQueueFamilyIndex = utils::getQueueFamilyIndex(_vrd.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
    _vrd.device = Factory::createDevice(_vrd.physicalDevice, _vrd.graphicsQueueFamilyIndex, features);

    // create surface
    VK_CHECK(glfwCreateWindowSurface(_vrd.instance, Application::getApp()->getWindow(), nullptr, &_surface));

    // make sure the graphics queue supports presentation
    VkBool32 presentationSupport;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(_vrd.physicalDevice, _vrd.graphicsQueueFamilyIndex, _surface, &presentationSupport));
    VK_ASSERT(presentationSupport == VK_TRUE, "Graphics queue does not support presentation");

    // retreive queue handle
    vkGetDeviceQueue(_vrd.device, _vrd.graphicsQueueFamilyIndex, 0, &_vrd.graphicsQueue);

    // get the max sample count. Note : we could decrease this number for better performance
    _vrd.sampleCount = utils::getMaximumSampleCount(_vrd.physicalDevice);

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

    // create color buffer attachment
    _colorBuffer.format = surfaceFormat.format;
    std::tie(_colorBuffer.image, _colorBuffer.deviceMemory)
            = Factory::createImage(&_vrd, _vrd.sampleCount, _swapchainExtent.width, _swapchainExtent.height, _colorBuffer.format,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                               //VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // TODO : change for transient?
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _colorBuffer.imageView = Factory::createImageView(_vrd.device, _colorBuffer.image, _colorBuffer.format, VK_IMAGE_ASPECT_COLOR_BIT);

    // create depth buffer attachment
    _depthBuffer.format = utils::findSupportedFormat(_vrd.physicalDevice,
                                                     {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                     VK_IMAGE_TILING_OPTIMAL,
                                                     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    std::tie(_depthBuffer.image, _depthBuffer.deviceMemory) = Factory::createImage(&_vrd, _vrd.sampleCount, _swapchainExtent.width,
               _swapchainExtent.height,_depthBuffer.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _depthBuffer.imageView = Factory::createImageView(_vrd.device, _depthBuffer.image, _depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT);

    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // we want to be able to individually reset a command buffer to its initial state
        .queueFamilyIndex = _vrd.graphicsQueueFamilyIndex
    };
    VK_CHECK(vkCreateCommandPool(_vrd.device, &commandPoolCreateInfo, nullptr, &_vrd.commandPool));

    // allocate command buffers from created command pool
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = _vrd.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    VK_CHECK(vkAllocateCommandBuffers(_vrd.device, &allocateInfo, _vrd.commandBuffers.data()));

    // create the main render pass
    createRenderPass(surfaceFormat.format);

    // push all layers
    _renderLayers.push_back(std::make_shared<ModelLayer>(_renderPass));
    _renderLayers.push_back(std::make_shared<LineLayer>(_renderPass));
    _renderLayers.push_back(std::make_shared<MultiMeshLayer>(_renderPass));
    _renderLayers.push_back(std::make_shared<FlipbookLayer>(_renderPass));
    _imGuiLayer = std::make_shared<ImGuiLayer>(_renderPass);
    _renderLayers.push_back(_imGuiLayer);

    // create framebuffers
    for (int i = 0; i < FB_COUNT; ++i) {
        std::array<VkImageView, 3> attachments = {_colorBuffer.imageView, _depthBuffer.imageView, _swapchainImageViews[i]};
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
    _renderFinishedFence = Factory::createFence(_vrd.device, true); // starts signaled
    for (auto& semaphore : _imageAvailSpres)
        semaphore = Factory::createSemaphore(_vrd.device);
    for (auto& semaphore : _renderFinishedSpres)
        semaphore = Factory::createSemaphore(_vrd.device);

    return true;
}

VulkanRenderDevice* Renderer::getRenderDevice() {
    return &_vrd;
}

VkExtent2D Renderer::getSwapchainExtent() {
    return _swapchainExtent;
}

void Renderer::onEvent(Event& e) {
    if (!_imguiFocus)
        _camera.onEvent(e);
    for (auto layer : _renderLayers)
        layer->onEvent(e);
}

Camera* Renderer::getCamera() {
    return &_camera;
}

void Renderer::update(float dt) {
    _fpsCounter.tick(dt, true); // TODO : remove hard coded true!
    if (!_imguiFocus)
        _camera.update(dt);
}


void Renderer::draw() {
    OPTICK_EVENT();
    // wait until the fence is signaled (ready to use)
    VK_CHECK(vkWaitForFences(_vrd.device, 1, &_renderFinishedFence, VK_TRUE, UINT64_MAX));

    // then unsignal the fence for next use
    VK_CHECK(vkResetFences(_vrd.device, 1, &_renderFinishedFence));

    uint32_t imageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_vrd.device, _swapchain, UINT64_MAX, _imageAvailSpres[_currentFiFIndex], nullptr, &imageIndex));

    // update render layers with delta time
    static double time = 0.f, lastFrame = 0.f;
    time = glfwGetTime();
    glm::mat4 pv = *_camera.getPVMatrix();
    for (auto layer : _renderLayers)
        layer->update(time - lastFrame, _currentFiFIndex, pv);
    lastFrame = time;

    _imGuiLayer->begin();
    onImGuiRender();
    for (auto layer : _renderLayers)
        layer->onImGuiRender();
    _imGuiLayer->end();

    // record command buffer at image index No need to reset the command buffer, beginCommandBuffer does it implicitally
    recordCommandBuffer(_currentFiFIndex, _frameBuffers[imageIndex]);

    // semaphore check to occur before writing to the color attachment
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_imageAvailSpres[_currentFiFIndex], // wait until signaled before starting
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &_vrd.commandBuffers[_currentFiFIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &_renderFinishedSpres[_currentFiFIndex] // signaled when command buffer is done executing
    };
    // the fence will be signaled once all commands have completed execution
    VK_CHECK(vkQueueSubmit(_vrd.graphicsQueue, 1, &submitInfo, _renderFinishedFence));

    // present to the screen once done rendering
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_renderFinishedSpres[_currentFiFIndex], // wait until frame in flight at _indexFif is done rendering
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    VK_CHECK(vkQueuePresentKHR(_vrd.graphicsQueue, &presentInfo));

    // alternate index of the frame in flight currently being rendered
    _currentFiFIndex = !_currentFiFIndex;

    // wait for completion of all operation on graphics queue (not optimal, but good enough for now)
    //VK_CHECK(vkDeviceWaitIdle(_vrd.device));
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
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &_vrd.instance));

#ifdef VELCRO_DEBUG
    Factory::setupDebugCallbacks(_vrd.instance, &_messenger, &_reportCallback);
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
    std::array<VkAttachmentDescription, 3> attachments{};

    // attachment associated with _colorBuffer. It is a multisampled buffer that we first render too.
    // We will then resolove this buffer to a single sampled buffer (in swapchain) to present to screen
    attachments[0] = {
      .flags = 0u,
      .format = swapchainFormat,
      .samples = _vrd.sampleCount,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // operation on color and depth at beginning of subpass : clear color buffer
      //  operation after subpass. We don't care since the image to be presented will be in the singled sampled swapchain buffer
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // we don't use stencil
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,  // we don't use stencil
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout of the image subressource when subpass begin. We don't care ; we clear it anyway
      // multisampled images cannot be presented directly. They are first resolved to an image then presented
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // FIXME : This could probably be undefined as well since we don't use the attachment after rendering
    };

    VkAttachmentReference colorRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    // attachment associated with depth buffer
    attachments[1] = {
        .flags = 0u,
        .format = _depthBuffer.format,
        .samples = _vrd.sampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,      // clear depth buffer at beginning of subpass
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // layout to be transitioned automatically when render pass instance ends
    };

    VkAttachmentReference depthRef = {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    // attachment associated with color image of the swapchain that will be presented
    attachments[2] = {
        .flags = 0u,
        .format = swapchainFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,          // must be singled sampled for presentation
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // we don't care since : (I think) : every pixel will be overwritten anyway when resolving from multisample -> single sampled
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,   // Store the image for presentation
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout of attachment at beggining of subpass
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // image ready for swapchain usage
    };

    // resolving from multi sample -> single sample in order to be presented
    VkAttachmentReference colorAttachmentResolve = {
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    // a render pass can consist of multiple subpasses. In our case we only use 1
    VkSubpassDescription subpassDescription = {
        .flags = 0u,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0u,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,  // The index of the attachment in this array is directly referenced from 
                                         //the fragment shader with the layout(location = 0) out vec4 outColor direct
        .pResolveAttachments = &colorAttachmentResolve,
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

void Renderer::recordCommandBuffer(uint32_t commandBufferIndex, VkFramebuffer framebuffer){
    
    // begin recording command
    VkCommandBufferBeginInfo commandBufferCI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    // begin command buffer implicitally resets the commands buffer : https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateFlagBits.html
    VK_CHECK(vkBeginCommandBuffer(_vrd.commandBuffers[commandBufferIndex], &commandBufferCI));

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
            },
    };
    VkRenderPassBeginInfo beginCI = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = _renderPass,
        .framebuffer = framebuffer,
        .renderArea = renderArea,
        .clearValueCount = sizeof(clearValues)/sizeof(VkClearValue),
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(_vrd.commandBuffers[commandBufferIndex], &beginCI, VK_SUBPASS_CONTENTS_INLINE);

    // record render commands from all the layers
    for (auto layer : _renderLayers)
        layer->fillCommandBuffer(_vrd.commandBuffers[commandBufferIndex], commandBufferIndex);

    // end the render pass
    vkCmdEndRenderPass(_vrd.commandBuffers[commandBufferIndex]);

    // stop recording commands
    VK_CHECK(vkEndCommandBuffer(_vrd.commandBuffers[commandBufferIndex]));
}

void Renderer::onImGuiRender() {
    // check if imgui wants capture (used to block event propagation)
    ImGuiIO& io = ImGui::GetIO();
    _imguiFocus = io.WantCaptureMouse;

    ImGui::Begin("Hello from Renderer");
    ImGui::Text("FPS %.2f", _fpsCounter.getFPS());
    if (ImGui::Button("Reset Camera"))
        _camera.reset();
    ImGui::End();
}