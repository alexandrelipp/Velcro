//
// Created by alexa on 2022-03-04.
//

#include "FactoryVulkan.h"

#include "../../Application.h"
#include "../../Utils/UtilsVulkan.h"

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

namespace Factory{


    bool setupDebugCallbacks(VkInstance instance,
                             VkDebugUtilsMessengerEXT* messenger,
                             VkDebugReportCallbackEXT* reportCallback) {

        // NOTE : we could just use volk load instead or link dynamically
        // The debugging functions from debug_report_ext are not part of the Vulkan core.
        // You need to dynamically load them from the instance via vkGetInstanceProcAddr
        // after making sure that it's actually supported:
        // https://stackoverflow.com/questions/37900051/vkcreatedebugreportcallback-ext-not-linking-but-every-other-functions-in-vulkan

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        auto vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");


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
                            VkDebugReportCallbackEXT reportCallback){
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

        vkDestroyDebugReportCallbackEXT(instance, reportCallback, nullptr);
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }

    VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex) {
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

        // gpu feature to be enabled (all disabled by default)
        VkPhysicalDeviceFeatures features{};
        features.geometryShader = VK_TRUE;

        const std::vector<const char*> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (auto e : extensions){
            VK_ASSERT(utils::isDeviceExtensionSupported(physicalDevice, e), "Device extension not supported");
        }

        // create logical device
        VkDeviceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .queueCreateInfoCount = 1u,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledLayerCount = 0u,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = (uint32_t)extensions.size(),
                .ppEnabledExtensionNames = extensions.data(),
                .pEnabledFeatures = &features,
        };
        VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
        return device;
    }
    VkSwapchainKHR createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, 
        VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, uint32_t framebufferCount)
    {
        VkSwapchainKHR swapchain;

        VkSurfaceCapabilitiesKHR capabilites;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilites));

        // get the swap chain extent. The FB size is required if surface capabilites does not contain valid value (unlikely)
        int FBwidth, FBheight;
        glfwGetFramebufferSize(Application::getApp()->getWindow(), &FBwidth, &FBheight);
        VkExtent2D swapchainExtent = utils::pickSwapchainExtent(capabilites, FBwidth, FBheight);

        // we request to have at least one more FB then the min image count, to prevent waiting on GPU
        SPDLOG_INFO("Min image count with current GPU and surface {}", capabilites.minImageCount);
        VK_ASSERT(capabilites.minImageCount < framebufferCount, "Number wrong");

        VkSwapchainCreateInfoKHR createInfo = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0u,
                .surface = surface,
                .minImageCount = framebufferCount,
                .imageFormat = surfaceFormat.format,
                .imageColorSpace = surfaceFormat.colorSpace,
                .imageExtent = swapchainExtent,
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
        VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain));
        return swapchain;
    }
}
