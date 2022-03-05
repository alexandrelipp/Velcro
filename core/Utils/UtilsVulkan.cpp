//
// Created by alexa on 2022-03-04.
//

#include "UtilsVulkan.h"
#include <vulkan/vulkan.h>

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
}



