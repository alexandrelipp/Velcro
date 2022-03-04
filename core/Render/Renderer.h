//
// Created by alexa on 2022-03-02.
//

#pragma once

#include <vulkan/vulkan.h>


class Renderer {
public:
    Renderer();
    bool init();

private:
    static bool isInstanceExtensionSupported(const char* extension);

private:
    VkInstance _instance = nullptr;
};



