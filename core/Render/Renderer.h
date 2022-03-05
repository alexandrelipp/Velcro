//
// Created by alexa on 2022-03-02.
//

#pragma once

#include <vulkan/vulkan.h>


class Renderer {
public:
    Renderer();
    ~Renderer();
    bool init();

private:
    void createInstance();

private:

    VkInstance _instance = nullptr;
    VkPhysicalDevice _physicalDevice = nullptr;
    VkDevice _device = nullptr; ///< Logical device

    VkSurfaceKHR _surface = nullptr;

    /// ONLY PRESENT IN DEBUG ///
#ifdef VELCRO_DEBUG
    VkDebugUtilsMessengerEXT _messenger = nullptr;
    VkDebugReportCallbackEXT _reportCallback = nullptr;
#endif
};



