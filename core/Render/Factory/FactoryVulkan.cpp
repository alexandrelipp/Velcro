//
// Created by alexa on 2022-03-04.
//

#include "FactoryVulkan.h"
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

    VkDevice createDevice(VkPhysicalDevice physicalDevice) {
        VkDevice device;
        // queue create info for the graphics queue
        float priority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0u,
                .queueFamilyIndex = utils::getQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT),
                .queueCount = 1,
                .pQueuePriorities = &priority,
        };

        // gpu feature to be enabled (all disabled by default)
        VkPhysicalDeviceFeatures features{};
        features.geometryShader = VK_TRUE;

        const std::vector<const char*> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

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
}
