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
    VK_CHECK(vkDeviceWaitIdle(_device));

    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_device);

    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

    // destroy sync objects
    //vkDestroyFence(_device, _inFlightFence, nullptr);
    vkDestroySemaphore(_device, _imageAvailSemaphore, nullptr);
    vkDestroySemaphore(_device, _renderFinishedSemaphore, nullptr);

    for (auto fb : _frameBuffers)
        vkDestroyFramebuffer(_device, fb, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyPipeline(_device, _graphicsPipeline, nullptr);

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

    // retreive queue handle
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);

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
        .flags = 0,
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
   
    createRenderPass(surfaceFormat.format);

    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_device, _physicalDevice, sizeof(mvp));

    createPipelineLayout();
    createDescriptorSets();

    ShaderFiles files = {
       .vertex = "vert.spv",
       .fragment = "frag.spv"
    };

    _graphicsPipeline = Factory::createGraphicsPipeline(_device, _swapchainExtent, _renderPass, _pipelineLayout, files);

    // create framebuffers
    for (int i = 0; i < FB_COUNT; ++i) {
        VkFramebufferCreateInfo framebufferCI = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .flags = 0u,
        .renderPass = _renderPass,
        .attachmentCount = 1,
        .pAttachments = &_swapchainImageViews[i],
        .width = _swapchainExtent.width,
        .height = _swapchainExtent.height,
        .layers = 1,
        };
        VK_CHECK(vkCreateFramebuffer(_device, &framebufferCI, nullptr, &_frameBuffers[i]));
    }

    // create sync objects
    //_inFlightFence = Factory::createFence(_device, true); // starts signaled
    _imageAvailSemaphore = Factory::createSemaphore(_device);
    _renderFinishedSemaphore = Factory::createSemaphore(_device);
  
    return true;
}

void Renderer::draw() {
    // wait until the fence is signaled (ready to use)
    //VK_CHECK(vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX));

    // then unsignal the fence for next use
    //VK_CHECK(vkResetFences(_device, 1, &_inFlightFence));

    static float angle = 0.f;
    angle += 0.0001f;

    mvp = glm::rotate(glm::mat4(1.f), angle, {0.f, 0.f, 1.f});

    uint32_t imageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailSemaphore, nullptr, &imageIndex));

    VK_ASSERT(_mvpUniformBuffers[imageIndex].setData(_device, glm::value_ptr(mvp), sizeof(mvp)), "Failed to update");
    //VK_CHECK(vkResetCommandBuffer(_commandBuffers[imageIndex], 0));
    // reset the command pool, probably not optimal but good enough for now
    VK_CHECK(vkResetCommandPool(_device, _commandPool, 0));
    recordCommandBuffer(imageIndex);

    // semaphore check to occur before writing to the color attachment
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_imageAvailSemaphore, // wait until signaled before starting
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &_renderFinishedSemaphore // signaled when command buffer is done executing
    };
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, nullptr));


    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    // wait for completion of all operation on graphics queue (not optimal, but good enough for now)
    VK_CHECK(vkDeviceWaitIdle(_device));
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
            .queueFamilyIndexCount = 0u,                            // only relevant when sharing mode is Concurrent
            .pQueueFamilyIndices = nullptr,                         // only relevant when sharing mode is Concurrent
            .preTransform = capabilites.currentTransform,           // Transform applied to image before presentation
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,    // blending mode with other surfaces
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
    };
    VK_CHECK(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain));
}

void Renderer::createRenderPass(VkFormat swapchainFormat){
    VkAttachmentDescription colorAttachment = {
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

    // a render pass can consist of multiple subpasses. In our case we only use 1
    VkSubpassDescription subpassDescription = {
        .flags = 0u,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0u,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,  // The index of the attachment in this array is directly referenced from 
                                         //the fragment shader with the layout(location = 0) out vec4 outColor directive!
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
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = (uint32_t)dependencies.size(),
        .pDependencies = dependencies.data()
    };

    VK_CHECK(vkCreateRenderPass(_device, &renderPassCI, nullptr, &_renderPass));
}

void Renderer::createPipelineLayout(){

    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &_descriptorSetLayout));
   

    // create pipeline layout (used for uniform and push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void Renderer::createDescriptorSets() {
    _descriptorPool = Factory::createDescriptorPool(_device, FB_COUNT, 1, 0, 0);

    std::array<VkDescriptorSetLayout, FB_COUNT> layouts = {_descriptorSetLayout, _descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_device, &descriptorSetAI, _descriptorSets.data()));

    for (size_t i = 0; i < _descriptorSets.size(); ++i){
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _mvpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _mvpUniformBuffers[i].getSize()
        };

        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        writeDescriptorSets.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = _descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo
        });

        VkDescriptorBufferInfo verticesInfo = {
                .buffer = _vertices.getBuffer(),
                .offset = 0,
                .range = _vertices.getSize()
        };

        writeDescriptorSets.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = _descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &verticesInfo
        });

        vkUpdateDescriptorSets(_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}


void Renderer::recordCommandBuffer(uint32_t index){
    
    // begin recording command
    VkCommandBufferBeginInfo commandBufferCI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK(vkBeginCommandBuffer(_commandBuffers[index], &commandBufferCI));

    // being render pass
    VkRect2D renderArea = {
        .offset = {
            .x = 0,
            .y = 0
        },
        .extent = _swapchainExtent
    };

    // TODO : is there a way to type pun this?
    VkClearValue clearValue = { .color = { _clearValue.r, _clearValue.g, _clearValue.b, _clearValue.a} };
    VkRenderPassBeginInfo beginCI = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = _renderPass,
        .framebuffer = _frameBuffers[index],
        .renderArea = renderArea,
        .clearValueCount = 1,
        .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(_commandBuffers[index], &beginCI, VK_SUBPASS_CONTENTS_INLINE);

    // bind pipeline and render 
    vkCmdBindPipeline(_commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(_commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[index], 0, nullptr);

    vkCmdDraw(_commandBuffers[index], 3, 1, 0, 0);

    // end the render pass
    vkCmdEndRenderPass(_commandBuffers[index]);

    // stop recording commands
    VK_CHECK(vkEndCommandBuffer(_commandBuffers[index]));
}