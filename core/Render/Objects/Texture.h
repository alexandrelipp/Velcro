//
// Created by alexa on 2022-03-15.
//

#pragma once

#include <vulkan/vulkan.h>


class Texture {
public:
    Texture() = default;

    void init(const std::string& filePath, VkDevice device, VkPhysicalDevice physicalDevice,
              VkQueue queue, VkCommandPool commandPool);

    void destroy(VkDevice device);

    VkSampler getSampler();
    VkImageView getImageView();

private:
    VkImage _image = nullptr;
    VkImageView _imageView = nullptr;
    VkDeviceMemory _imageMemory = nullptr;
    VkSampler _sampler = nullptr; // TODO : do we need one sampler per texture?
};



