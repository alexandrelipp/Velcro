//
// Created by alexa on 2022-03-04.
//

#include "UtilsVulkan.h"

namespace utils {
    bool isInstanceExtensionSupported(const char* extension) {
        uint32_t count;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        std::vector<VkExtensionProperties> extensions(count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
        for (auto& e: extensions) {
            //SPDLOG_INFO("Extensions {}", e.extensionName);
            if (strcmp(e.extensionName, extension) == 0)
                return true;
        }
        return false;
    }

    bool isInstanceLayerSupported(const char* layer) {
        uint32_t count;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
        std::vector<VkLayerProperties> layers(count);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&count, layers.data()));
        for (auto l: layers) {
            SPDLOG_INFO("Layer {}", l.layerName);
            // SPDLOG_INFO("Desc {}", l.description);
            if (strcmp(layer, l.layerName) == 0)
                return true;
        }
        return false;
    }

    VkPhysicalDevice pickPhysicalDevice(VkInstance instance, const VkPhysicalDeviceFeatures& features) {
        uint32_t count;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
        SPDLOG_INFO("Number of available physical devices {}", count);

        for (VkPhysicalDevice device: devices) {
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            // gpu must be dedicated (not virtual/integrated/cpu) and all requested features must be supported
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && utils::isFeaturesSupported(deviceFeatures, features))
                return device;
        }
        VK_ASSERT(false, "Failed to pick a physical device");
        return nullptr;
    }

    void printPhysicalDeviceProps(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        SPDLOG_INFO("GPU {} from vendor {}", props.deviceName, props.vendorID);
        SPDLOG_INFO("GPU Type {}", magic_enum::enum_name(props.deviceType));
    }

    bool isDeviceExtensionSupported(VkPhysicalDevice device, const char* extension) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        for (auto e: availableExtensions) {
            if (strcmp(e.extensionName, extension) == 0)
                return true;
        }
        return false;
    }

    bool isFeaturesSupported(const VkPhysicalDeviceFeatures& supportedFeatures,
                             const VkPhysicalDeviceFeatures& requestedFeatures) {
        VkBool32* supportedFeaturesPtr = (VkBool32*)&supportedFeatures;
        VkBool32* requestedFeaturesPtr = (VkBool32*)&requestedFeatures;

        // VkPhysicalDeviceFeatures is only composed of VkBool32
        uint32_t numFlags = sizeof(VkPhysicalDeviceFeatures)/sizeof(VkBool32);

        // iterate all the features and make sure all the requested features are supported
        for (uint32_t i = 0; i < numFlags; ++i){
            if (requestedFeaturesPtr[i] == VK_TRUE && supportedFeaturesPtr[i] != VK_TRUE)
                return false;
        }
        return true;
    }

    uint32_t getQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueFlags) {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

        for (int i = 0; i < properties.size(); ++i) {
            if (properties[i].queueFlags & queueFlags)
                return i;
        }
        VK_ASSERT(false, "Failed to find a queue family supporting graphics");
        return 0;
    }

    void printQueueFamiliesInfo(VkPhysicalDevice device) {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
        for (auto props: properties) {
            SPDLOG_INFO("Queue count {}", props.queueCount);
            std::string type;
            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                type += " Graphics";
            if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
                type += " Compute";
            if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
                type += " Transfer";
            if (props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                type += " SparseBinding";
            SPDLOG_INFO("Type {}", type);
        }
    }

    void executeOnQueueSync(VkQueue queue, VkDevice device, VkCommandPool pool, const std::function<void(VkCommandBuffer)>& commands) {
        // allocate command buffer
        VkCommandBufferAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
        };
        VkCommandBuffer commandBuffer = nullptr;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

        // begin command buffer
        VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT ,
        };

        // record commands
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        commands(commandBuffer);
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // submit command to queue
        VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = nullptr,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr
        };
        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, nullptr));

        // wait until the queue is idle (done executing commands), making this function synchronous
        VK_CHECK(vkQueueWaitIdle(queue));
        vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
    }


    VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        uint32_t count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data()));
        for (auto& format: formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }
        VK_ASSERT(false, "Failed to find surfaceFormat");
        return formats[0];
    }

    VkPresentModeKHR pickSurfacePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        uint32_t count;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> presentModes(count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModes.data()));

        // use mailbox if possible ; default to fifo if not present
        // TODO is it possible to MAILBOX with vsync??
//        for (auto mode: presentModes) {
//            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
//                return mode;
//        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilites, int frameBufferW, int frameBufferH) {
        // if current extent is at numeric limits, it can vary. Otherwise, it's the size of the window
        if (surfaceCapabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surfaceCapabilites.currentExtent;
        }

        // return new extent using window size
        VkExtent2D newExtent = {(uint32_t) frameBufferW, (uint32_t) frameBufferH};

        // clamp values to make sure extent size fits within the max surface extent
        newExtent.width = glm::clamp(newExtent.width, surfaceCapabilites.minImageExtent.width,
                                     surfaceCapabilites.maxImageExtent.height);
        newExtent.height = glm::clamp(newExtent.height, surfaceCapabilites.minImageExtent.width,
                                      surfaceCapabilites.maxImageExtent.height);

        return newExtent;
    }

    bool transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool pool, VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkImageMemoryBarrier memoryBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout = oldLayout,
                .newLayout = newLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = image,
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                }
        };

        // Stage flags that will be filled depending on the given layouts
        VkPipelineStageFlags srcStage, dstStage;

        // if transitioning from new image(UNDEFINED) to image ready to received data
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
            memoryBarrier.srcAccessMask = VK_ACCESS_NONE;               // memory access stage transition must after
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // memory access stage transition must before
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // transition must happen after the transfer stage is done writing
            memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            // transition must happen before the fragment shader tries to read the image
            memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            VK_ASSERT(false, "Non supported transfer formats");
            return false;
        }

        // use a pipeline barrier (between stages) to transition the layouts
        executeOnQueueSync(queue, device, pool, [&](VkCommandBuffer commandBuffer){
            vkCmdPipelineBarrier(commandBuffer,
                                 srcStage,                          // src stage (used with given srcAccessMask)
                                 dstStage,                          // dst stage (used with given dstAccessMask)
                                 0,                                 // dependency flags (could use by region)
                                 0, nullptr,                        // memory barriers
                                 0, nullptr,                        // buffer memory barriers
                                 1, &memoryBarrier                  // image barriers
            );
        });




        return true;
    }

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        // NOTE : we could also check the heap types in the mem properties
        for (size_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if (((1 << i) & typeFilter) &&
                // we want all flags, not only one (why we put the ==)
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        VK_ASSERT(false, "Failed to find a memory type");
        return 0;
    }

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            // depending on given tiling, need to check for different bit flags
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        throw std::runtime_error("failed to find supported format!");
    }

//    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
//        return findSupportedFormat( physicalDevice,
//                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
//                VK_IMAGE_TILING_OPTIMAL,
//                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
//        );
//    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkSampleCountFlagBits getMaximumSampleCount(VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        // we are using a depth buffer, so we have to take in account color and depth
        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT)  return VK_SAMPLE_COUNT_64_BIT;
        if (counts & VK_SAMPLE_COUNT_32_BIT)  return VK_SAMPLE_COUNT_32_BIT;
        if (counts & VK_SAMPLE_COUNT_16_BIT)  return VK_SAMPLE_COUNT_16_BIT;
        if (counts & VK_SAMPLE_COUNT_8_BIT)   return VK_SAMPLE_COUNT_8_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT)   return VK_SAMPLE_COUNT_4_BIT;
        if (counts & VK_SAMPLE_COUNT_2_BIT)   return VK_SAMPLE_COUNT_2_BIT;

        return VK_SAMPLE_COUNT_1_BIT;
    }
}
