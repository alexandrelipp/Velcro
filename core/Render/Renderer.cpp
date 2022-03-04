//
// Created by alexa on 2022-03-02.
//

#include "Renderer.h"
#include "../../Application.h"

// TODO : maybe add a forceDebug flag
#ifdef NDEBUG
#define VELCO_RELEASE
#else
#define VELCRO_DEBUG
#endif

// macro bs to create unique variable name using line number
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

#define VK_CHECK(result) auto UNIQUE_NAME(s) = result; \
    if (UNIQUE_NAME(s) != VK_SUCCESS) \
        handleError(UNIQUE_NAME(s))

static void handleError(VkResult result){
    SPDLOG_ERROR("Check success failed with code {}", magic_enum::enum_name(result));
#ifdef VELCRO_DEBUG
    throw std::runtime_error("VK FAILED");
#endif
}

#define VK_ASSERT(result, mes) if (!(result)) \
        throw std::runtime_error(mes);


Renderer::Renderer() {

}

bool Renderer::init() {
    std::vector<const char*> extensions;
    uint32_t count;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    for (int i = 0; i < count; ++i){
        VK_ASSERT(isInstanceExtensionSupported(glfwExtensions[i]), "Extension not supported");
        extensions.push_back(glfwExtensions[i]);
    }

    for (auto e : extensions){
        SPDLOG_INFO("Enabled instanced extension {}", e);
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
            .enabledLayerCount = 0u,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = (uint32_t)extensions.size(),
            .ppEnabledExtensionNames = extensions.data(),
        };
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &_instance));
    return true;
}

bool Renderer::isInstanceExtensionSupported(const char* extension) {
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
