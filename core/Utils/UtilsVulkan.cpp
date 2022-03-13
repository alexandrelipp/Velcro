//
// Created by alexa on 2022-03-04.
//

#include "UtilsVulkan.h"

namespace utils{
    bool isInstanceExtensionSupported(const char* extension) {
        uint32_t count;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        std::vector<VkExtensionProperties> extensions(count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
        for (auto& e : extensions){
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
        for (auto l : layers){
             SPDLOG_INFO("Layer {}", l.layerName);
            // SPDLOG_INFO("Desc {}", l.description);
            if (strcmp(layer,l.layerName) == 0)
                return true;
        }
        return false;
    }

    VkPhysicalDevice pickPhysicalDevice(VkInstance instance) {
        uint32_t count;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
        SPDLOG_INFO("Number of available physical devices {}", count);

        for (VkPhysicalDevice device : devices){
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            // geo shader must be supported and the gpu must be dedicated (not virtual/integrated/cpu)
            if (features.geometryShader && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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
        for (auto e : availableExtensions){
            if (strcmp(e.extensionName, extension) == 0)
                return true;
        }
        return false;
    }

    uint32_t getQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits queueFlags) {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

        for (int i = 0; i < properties.size(); ++i){
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
        for (auto props : properties){
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

    VkSurfaceFormatKHR pickSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        uint32_t count;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data()));
        for (auto& format : formats){
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
        for (auto mode : presentModes){
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D pickSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilites, int frameBufferW, int frameBufferH)
    {
        // if current extent is at numeric limits, it can vary. Otherwise it's the size of the window
        if (surfaceCapabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surfaceCapabilites.currentExtent;
        }

        // return new extent using window size
        VkExtent2D newExtent = { frameBufferW, frameBufferH };

        // clamp values to make sure extent size fits within the max surface extent
        newExtent.width = glm::clamp(newExtent.width, surfaceCapabilites.minImageExtent.width, surfaceCapabilites.maxImageExtent.height);
        newExtent.height = glm::clamp(newExtent.height, surfaceCapabilites.minImageExtent.width, surfaceCapabilites.maxImageExtent.height);

        return newExtent;
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
                .r = VK_COMPONENT_SWIZZLE_IDENTITY, // component used when swizzling : vec.rrr Identy means no change. Allows remaping
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

    int findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties){
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
        return -1;
    }
}
