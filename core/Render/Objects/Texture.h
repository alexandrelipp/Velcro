//
// Created by alexa on 2022-03-15.
//

#pragma once

#include "../VulkanRenderDevice.hpp"

#include <vulkan/vulkan.h>
#include <string>


class Texture {
public:
    Texture() = default;

    void init(const std::string& filePath, VulkanRenderDevice& renderDevice, bool createSampler);

    void destroy(VkDevice device);

    VkSampler getSampler();
    VkImageView getImageView();

private:
    VkImage _image = nullptr;
    VkImageView _imageView = nullptr;
    VkDeviceMemory _imageMemory = nullptr;
    VkSampler _sampler = nullptr; // TODO : we really want to store the sampler in the texture ??
};



