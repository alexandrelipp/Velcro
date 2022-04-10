//
// Created by alexa on 2022-03-04.
//

#include "FactoryVulkan.h"

#include "../../Application.h"
#include "../../Utils/UtilsVulkan.h"
#include "../../Utils/UtilsFile.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
        VkDebugUtilsMessageTypeFlagsEXT Type,
        const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
        void* UserData) {
    if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
        SPDLOG_TRACE("Validation Layer : {}", CallbackData->pMessage);
    }
    if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
        SPDLOG_INFO("Validation Layer : {}", CallbackData->pMessage);
    }
    if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
        SPDLOG_WARN("Validation Layer : {}", CallbackData->pMessage);
    }
    if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
        SPDLOG_ERROR("Validation Layer : {}", CallbackData->pMessage);
    }

    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback
        (
                VkDebugReportFlagsEXT      flags,
                VkDebugReportObjectTypeEXT objectType,
                uint64_t                   object,
                size_t                     location,
                int32_t                    messageCode,
                const char* pLayerPrefix,
                const char* pMessage,
                void* UserData) {
    // https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
    // This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        return VK_FALSE;

    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT){
        SPDLOG_DEBUG("Debug callback {} : {}", pLayerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT){
        SPDLOG_WARN("Debug callback {} : {}", pLayerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT){
        SPDLOG_INFO("Debug callback {} : {}", pLayerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT){
        SPDLOG_ERROR("Debug callback {} : {}", pLayerPrefix, pMessage);
    }

    return VK_FALSE;
}

namespace Factory {


    bool setupDebugCallbacks(VkInstance instance,
                             VkDebugUtilsMessengerEXT* messenger,
                             VkDebugReportCallbackEXT* reportCallback) {

        // NOTE : we could just use volk load instead or link dynamically
        // The debugging functions from debug_report_ext are not part of the Vulkan core.
        // You need to dynamically load them from the instance via vkGetInstanceProcAddr
        // after making sure that it's actually supported:
        // https://stackoverflow.com/questions/37900051/vkcreatedebugreportcallback-ext-not-linking-but-every-other-functions-in-vulkan

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) (vkGetInstanceProcAddr(instance,
                                                                                                          "vkCreateDebugUtilsMessengerEXT"));
        auto vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance,
                                                                                                      "vkCreateDebugReportCallbackEXT");


        const VkDebugUtilsMessengerCreateInfoEXT ci = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = &VulkanDebugCallback,
                .pUserData = nullptr
        };

        VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &ci, nullptr, messenger));
        const VkDebugReportCallbackCreateInfoEXT dci = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags =
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_DEBUG_BIT_EXT,
                .pfnCallback = &VulkanDebugReportCallback,
                .pUserData = nullptr
        };

        VK_CHECK(vkCreateDebugReportCallback(instance, &dci, nullptr, reportCallback));


        return true;
    }

    void freeDebugCallbacks(VkInstance instance,
                            VkDebugUtilsMessengerEXT messenger,
                            VkDebugReportCallbackEXT reportCallback) {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                                                           "vkDestroyDebugUtilsMessengerEXT");
        auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance,
                                                                                                           "vkDestroyDebugReportCallbackEXT");

        vkDestroyDebugReportCallbackEXT(instance, reportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }

    VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex,
                          const VkPhysicalDeviceFeatures& features) {
        VkDevice device;
        // queue create info for the graphics queue
        float priority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .queueFamilyIndex = graphicsQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &priority,
        };

        const std::vector<const char*> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (auto e: extensions) {
            VK_ASSERT(utils::isDeviceExtensionSupported(physicalDevice, e), "Device extension not supported");
        }

        // TODO : why do we need this to suppress validation warnings?
        VkPhysicalDeviceVulkan11Features features11 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                .shaderDrawParameters = VK_TRUE
        };

        // TODO : can we check if it is supported ?
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
                .pNext = &features11,
                .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                .descriptorBindingVariableDescriptorCount = VK_TRUE,
                .runtimeDescriptorArray = VK_TRUE
        };

        // create logical device
        VkDeviceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &indexingFeatures,
                .flags = 0u,
                .queueCreateInfoCount = 1u,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledLayerCount = 0u,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = (uint32_t) extensions.size(),
                .ppEnabledExtensionNames = extensions.data(),
                .pEnabledFeatures = &features,
        };
        VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
        return device;
    }

    VkSemaphore createSemaphore(VkDevice device) {
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u
        };
        VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &semaphore));
        return semaphore;
    }

    VkFence createFence(VkDevice device, bool startSignaled) {
        VkFenceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = (startSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u),
        };
        VkFence output = nullptr;
        VK_CHECK(vkCreateFence(device, &createInfo, nullptr, &output));
        return output;
    }

    VkShaderModule createShaderModule(VkDevice device, const std::string& filename) {
        VkShaderModule shaderModule = nullptr;
        std::filesystem::path path(R"(..\..\..\core\Assets\Shaders\bin)");
        path /= filename;
        VK_ASSERT(utils::fileExists(path), "Shader file does not exist");

        std::vector<char> code = utils::getFileContent(path.string());

        VkShaderModuleCreateInfo shaderCI = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .codeSize = (uint32_t) code.size(),
                .pCode = (uint32_t*) code.data(),
        };

        VK_CHECK(vkCreateShaderModule(device, &shaderCI, nullptr, &shaderModule));
        return shaderModule;
    }

    VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D& extent, VkRenderPass renderPass,
                                      VkPipelineLayout pipelineLayout, const GraphicsPipelineProps& props) {
        VK_ASSERT(!props.shaders.geometry.has_value(), "Geo shader not supported yet");

        VK_ASSERT(props.shaders.fragment.has_value() && props.shaders.vertex.has_value(), "Filenames are empty");
        VkShaderModule vertModule = Factory::createShaderModule(device, props.shaders.vertex.value());
        VkShaderModule fragModule = Factory::createShaderModule(device, props.shaders.fragment.value());

        std::array<VkPipelineShaderStageCreateInfo, 2> shadersCI{};

        shadersCI[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertModule,
                .pName = "main",
                .pSpecializationInfo = nullptr // useful to configure compile time constant
        };

        shadersCI[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragModule,
                .pName = "main",
                .pSpecializationInfo = nullptr // useful to configure compile time constant
        };

        VkPipelineVertexInputStateCreateInfo inputInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .vertexBindingDescriptionCount = 0u,
                .pVertexBindingDescriptions = nullptr,
                .vertexAttributeDescriptionCount = 0u,
                .pVertexAttributeDescriptions = nullptr,
        };

        VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .topology = props.primitiveTopology,
                .primitiveRestartEnable = VK_FALSE, // if enabled a special index value (0xFFFFFFFF) restarts the assembly if drawing indexed
        };

        VkViewport viewPort = {
                .x = 0.f,
                .y = 0.f,
                .width = (float) extent.width,
                .height = (float) extent.height,
                .minDepth = 0.f,
                .maxDepth = 1.f,
        };

        VkRect2D scissor = {
                .offset = {0, 0},
                .extent = extent
        };

        VkPipelineViewportStateCreateInfo viewPortCI = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .viewportCount = 1,
                .pViewports = &viewPort,
                .scissorCount = 1,
                .pScissors = &scissor
        };

        VkPipelineRasterizationStateCreateInfo rastCI = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .depthClampEnable = VK_FALSE, // if true, fragments beyond the near and far planes are clamped to them instead of discarded
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_NONE,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE, // rasterizer can alter the depth values
                .lineWidth = 1.f,            // line width (in pixels)
        };

        // set up multisampling (disabled for now)
        VkPipelineMultisampleStateCreateInfo multisampleCI = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilCI = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = props.enableDepthTest,
                .depthWriteEnable = props.enableDepthTest, // TODO : do we want to distinct write + enable depth test ?
                .depthCompareOp = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable = VK_FALSE,      // if enabled, depth test will only pass when inside the given bounds
                .stencilTestEnable = VK_FALSE,
        };

        // set up default color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
                .blendEnable = VK_FALSE,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                  VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT

        };

        // enable color blending if requested
        if (props.enableBlending){
            // these params accomplish these operations :
            // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
            // finalColor.a = newAlpha.a;
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendCI = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,  // alternate way of blending is by using a logic operation
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment,
                .blendConstants = {0.f, 0.f, 0.f, 0.f}
        };

        VkGraphicsPipelineCreateInfo pipelineCI = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = shadersCI.size(),
                .pStages = shadersCI.data(),
                .pVertexInputState = &inputInfo,
                .pInputAssemblyState = &assemblyInfo,
                .pViewportState = &viewPortCI,
                .pRasterizationState = &rastCI,
                .pMultisampleState = &multisampleCI,
                .pDepthStencilState = &depthStencilCI,
                .pColorBlendState = &colorBlendCI,
                .pDynamicState = nullptr, // not used for now
                .layout = pipelineLayout,
                .renderPass = renderPass,
                .subpass = 0, // index of subpass where the graphics pipeline willbe used
                .basePipelineHandle = nullptr,
                .basePipelineIndex = -1,

        };

        VkPipeline output = nullptr;
        VK_CHECK(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &output));

        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);

        return output;
    }

    std::pair<VkBuffer, VkDeviceMemory> createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                                                     VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBuffer buffer = nullptr;
        // create buffer (info about buffer, but a buffer does not contain data)
        VkBufferCreateInfo bufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .flags = 0u,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // only one queue can access
                .queueFamilyIndexCount = 0,         // only relevant if concurrent sharing mode
                .pQueueFamilyIndices = nullptr,     // only relevant if concurrent sharing mode
        };

        VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer));

        // allocate memory on physical device for buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        VkMemoryAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = utils::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
        };
        VkDeviceMemory bufferMemory = nullptr;
        VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory));

        // bind allocated memory to buffer
        VK_CHECK(vkBindBufferMemory(device, buffer, bufferMemory, 0));

        return std::make_pair(buffer, bufferMemory);
    }

    std::pair<VkImage, VkDeviceMemory> createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                                                   uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
        // create image
        VkImageCreateInfo imageCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags = 0u,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = {
                        .width = width,
                        .height = height,
                        .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = tiling,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        VkImage image = nullptr;
        VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

        // allocate memory
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, image, &memoryRequirements);
        VkMemoryAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memoryRequirements.size,
                .memoryTypeIndex = utils::findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties)
        };

        VkDeviceMemory deviceMemory = nullptr;
        VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &deviceMemory));

        // bind image to memory
        VK_CHECK(vkBindImageMemory(device, image, deviceMemory, 0));

        return std::make_pair(image, deviceMemory);
    }

    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageView imageView = nullptr;
        const VkImageViewCreateInfo viewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY, // component used when swizzling : vec.rrr Identity means no change. Allows remapping
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                        .aspectMask = aspectFlags,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                }
        };

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
        return imageView;
    }

    VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t imageCount,
                                                           uint32_t uniformBufferCount,
                                                           uint32_t storageBufferCount,
                                                           uint32_t samplerImageCount) {

        std::vector<VkDescriptorPoolSize> poolSizes;
        if (uniformBufferCount){
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = uniformBufferCount * imageCount
            });
        }
        if (storageBufferCount){
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = storageBufferCount * imageCount
            });
        }

        if (samplerImageCount){
            poolSizes.push_back(VkDescriptorPoolSize{
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = samplerImageCount * imageCount
            });
        }

        VkDescriptorPoolCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = imageCount,
                .poolSizeCount = (uint32_t)poolSizes.size(),
                .pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
        };
        VkDescriptorPool output = nullptr;
        VK_CHECK(vkCreateDescriptorPool(device, &createInfo, nullptr, &output));
        return output;
    }

    std::tuple<VkDescriptorSetLayout, VkPipelineLayout, VkDescriptorPool, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>>
    Factory::createDescriptorSets(VulkanRenderDevice* renderDevice, const std::vector<Descriptor>& descriptors,
                                  const std::vector<VkPushConstantRange>& pushConstants) {

        // temp variables used to count descriptor count by type
        uint32_t uniformBufferCount = 0, storageBufferCount = 0, samplerImageCount = 0;

        // create layout bindings
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptors.size());
        for (uint32_t i = 0; i < descriptors.size(); ++i){
            // imageInfos will be nullptr if info is not of type vector(DescriptorImageInfo)
            auto* imageInfos = std::get_if<std::vector<VkDescriptorImageInfo>>(&descriptors[i].info);
            layoutBindings[i] = {
                    .binding = i,
                    .descriptorType = descriptors[i].type,
                    .descriptorCount = imageInfos == nullptr ? 1 : (uint32_t)imageInfos->size(),
                    .stageFlags = descriptors[i].shaderStage
            };

            // add type of descriptor to descriptor count (only samplerImages can be arrays)
            switch (descriptors[i].type) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    uniformBufferCount++;
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    storageBufferCount++;
                    break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    // imageInfos must exist if of type image sampler!
                    samplerImageCount += imageInfos->size();
                    break;
                default:
                    VK_ASSERT(false, "Descriptor's type not supported : " + std::string(magic_enum::enum_name(descriptors[i].type)));
            }
        }

        // create the descriptor set layout
        VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = (uint32_t)layoutBindings.size(),
                .pBindings = layoutBindings.data()
        };
        VkDescriptorSetLayout descriptorSetLayout = nullptr;
        VK_CHECK(vkCreateDescriptorSetLayout(renderDevice->device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

        // create pipeline layout (descriptor set layout + push constant layout)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

        VkPipelineLayout pipelineLayout = nullptr;
        VK_CHECK(vkCreatePipelineLayout(renderDevice->device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // create descriptor pool
        // Inadequate descriptor pools are a good example of a problem that the validation layers will not catch:
        // As of Vulkan 1.1, vkAllocateDescriptorSets may fail with the error code VK_ERROR_POOL_OUT_OF_MEMORY
        // if the pool is not sufficiently large, but the driver may also try to solve the problem internally.
        // This means that sometimes (depending on hardware, pool size and allocation size) the driver will let
        // us get away with an allocation that exceeds the limits of our descriptor pool.
        // Other times, vkAllocateDescriptorSets will fail and return VK_ERROR_POOL_OUT_OF_MEMORY.
        // This can be particularly frustrating if the allocation succeeds on some machines, but fails on others.
        VkDescriptorPool descriptorPool = Factory::createDescriptorPool(renderDevice->device, MAX_FRAMES_IN_FLIGHT,
                                                                        uniformBufferCount, storageBufferCount, samplerImageCount);

        std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = {descriptorSetLayout, descriptorSetLayout};

        // allocate descriptors from descriptor pool
        VkDescriptorSetAllocateInfo descriptorSetAI = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = descriptorPool,
                .descriptorSetCount = (uint32_t)layouts.size(),
                .pSetLayouts = layouts.data()
        };
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets{};
        VK_CHECK(vkAllocateDescriptorSets(renderDevice->device, &descriptorSetAI, descriptorSets.data()));

        // update the descriptor sets with the ressources handles
        for (uint32_t i = 0; i < descriptorSets.size(); ++i) {
            // create write descriptor set for each descriptor set
            std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptors.size());

            for (uint32_t j = 0; j < descriptors.size(); ++j){
                auto& descWrite = writeDescriptorSets[j];
                descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descWrite.dstSet = descriptorSets[i];
                descWrite.dstBinding = j;
                descWrite.dstArrayElement = 0;
                descWrite.descriptorType = descriptors[j].type;

                auto* imageInfo = std::get_if<std::vector<VkDescriptorImageInfo>>(&descriptors[j].info);
                if (imageInfo != nullptr){
                    descWrite.descriptorCount = imageInfo->size();
                    descWrite.pImageInfo = imageInfo->data();
                }
                else {
                    descWrite.descriptorCount = 1;
                    descWrite.pBufferInfo = &std::get<std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>>(descriptors[j].info)[i];
                }
            }

            // update the descriptor sets with the created descriptor writes
            vkUpdateDescriptorSets(renderDevice->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
        }

        // return all the handles
        return std::make_tuple(descriptorSetLayout, pipelineLayout, descriptorPool, descriptorSets);
    }
}
