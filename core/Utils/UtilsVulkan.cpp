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
            // SPDLOG_INFO("Layer {}", l.layerName);
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

    uint32_t getGraphicsQueueFamilyIndex(VkPhysicalDevice device) {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

        for (int i = 0; i < properties.size(); ++i){
            if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
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

}



