//
// Created by alexa on 2022-03-02.
//

#include "Renderer.h"
#include "../../Application.h"

#include "../Utils/UtilsVulkan.h"
#include "../Utils/UtilsFile.h"
#include "Factory/FactoryVulkan.h"
#include "Factory/FactoryModel.h"


Renderer::Renderer() {

}

Renderer::~Renderer() {
    VK_CHECK(vkDeviceWaitIdle(_device));

    // destroy the buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_device);
    _vertices.destroy(_device);
    _indices.destroy(_device);

    _texture.destroy(_device);

    // destroy descriptors
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

    // free depth buffer
    vkDestroyImageView(_device, _depthBuffer.imageView, nullptr);
    vkDestroyImage(_device, _depthBuffer.image, nullptr);
    vkFreeMemory(_device, _depthBuffer.deviceMemory, nullptr);

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
        _swapchainImageViews[i] = Factory::createImageView(_device, _swapchainImages[i], surfaceFormat.format, flags);
    }

    _depthBuffer.format = utils::findSupportedFormat( _physicalDevice,
               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
               VK_IMAGE_TILING_OPTIMAL,
               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    auto depth = Factory::createImage(_device, _physicalDevice, _swapchainExtent.width, _swapchainExtent.height,
                                      _depthBuffer.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _depthBuffer.image = depth.first;
    _depthBuffer.deviceMemory = depth.second;

    _depthBuffer.imageView = Factory::createImageView(_device, depth.first, _depthBuffer.format, VK_IMAGE_ASPECT_DEPTH_BIT);

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

    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_device, _physicalDevice, sizeof(mvp));

    // create duck model
    std::vector<TexVertex> vertices;
    std::vector<uint32_t> indices;
    VK_ASSERT(FactoryModel::createDuckModel(vertices, indices), "Failed to create mesh");
    _indexCount = indices.size();

    // init the vertices ssbo
    _vertices.init(_device, _physicalDevice, vertices.size() * sizeof(vertices[0]));
    VK_ASSERT(_vertices.setData(_device, _physicalDevice, _graphicsQueue, _commandPool,
                                vertices.data(), vertices.size() * sizeof(vertices[0])), "set data failed");

    // init the indices ssbo
    _indices.init(_device, _physicalDevice, sizeof(indices[0]) * indices.size());
    VK_ASSERT(_indices.setData(_device, _physicalDevice, _graphicsQueue, _commandPool,
                               indices.data(), indices.size() * sizeof(indices[0])), "set data failed");

    // init the statue texture
    _texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", _device, _physicalDevice, _graphicsQueue, _commandPool);

    createPipelineLayout();
    createDescriptorSets();

    ShaderFiles files = {
       .vertex = "vert.spv",
       .fragment = "frag.spv"
    };

    _graphicsPipeline = Factory::createGraphicsPipeline(_device, _swapchainExtent, _renderPass, _pipelineLayout, files);

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
        VK_CHECK(vkCreateFramebuffer(_device, &framebufferCI, nullptr, &_frameBuffers[i]));
    }

    // create sync objects
    //_inFlightFence = Factory::createFence(_device, true); // starts signaled
    _imageAvailSemaphore = Factory::createSemaphore(_device);
    _renderFinishedSemaphore = Factory::createSemaphore(_device);
  
    return true;
}

VkDevice Renderer::getDevice() {
    return _device;
}

VkExtent2D Renderer::getSwapchainExtent() {
    return _swapchainExtent;
}


void Renderer::draw() {
    // wait until the fence is signaled (ready to use)
    //VK_CHECK(vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX));

    // then unsignal the fence for next use
    //VK_CHECK(vkResetFences(_device, 1, &_inFlightFence));

    static float angle = 0.f;
    //angle += 0.0001f;

    float aspectRatio = _swapchainExtent.width/(float)_swapchainExtent.height;
    glm::mat4 v = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 p = glm::perspective(45.f, aspectRatio, 0.1f, 1000.f);
    glm::mat4 m = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.5f, -1.5f)) * glm::rotate(glm::mat4(1.f), glm::pi<float>(),
                                                                             glm::vec3(1, 0, 0)),
            (float)glfwGetTime(),
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

     mvp = p  * v * m;

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

    VK_CHECK(vkCreateRenderPass(_device, &renderPassCI, nullptr, &_renderPass));
}

void Renderer::createPipelineLayout(){
    std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings = {
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = layoutBindings.size(),
        .pBindings = layoutBindings.data()
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
    // Inadequate descriptor pools are a good example of a problem that the validation layers will not catch:
    // As of Vulkan 1.1, vkAllocateDescriptorSets may fail with the error code VK_ERROR_POOL_OUT_OF_MEMORY
    // if the pool is not sufficiently large, but the driver may also try to solve the problem internally.
    // This means that sometimes (depending on hardware, pool size and allocation size) the driver will let
    // us get away with an allocation that exceeds the limits of our descriptor pool.
    // Other times, vkAllocateDescriptorSets will fail and return VK_ERROR_POOL_OUT_OF_MEMORY.
    // This can be particularly frustrating if the allocation succeeds on some machines, but fails on others.
    _descriptorPool = Factory::createDescriptorPool(_device, FB_COUNT, 1, 2, 0);

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

        VkDescriptorBufferInfo indicesInfo = {
                .buffer = _indices.getBuffer(),
                .offset = 0,
                .range = _indices.getSize()
        };

        writeDescriptorSets.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = _descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &indicesInfo
        });

        VkDescriptorImageInfo imageInfo = {
                .sampler = _texture.getSampler(),
                .imageView = _texture.getImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        writeDescriptorSets.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = _descriptorSets[i],
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
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
    vkCmdBeginRenderPass(_commandBuffers[index], &beginCI, VK_SUBPASS_CONTENTS_INLINE);

    // bind pipeline and render 
    vkCmdBindPipeline(_commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(_commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[index], 0, nullptr);

    vkCmdDraw(_commandBuffers[index], _indexCount, 1, 0, 0);

    // end the render pass
    vkCmdEndRenderPass(_commandBuffers[index]);

    // stop recording commands
    VK_CHECK(vkEndCommandBuffer(_commandBuffers[index]));
}