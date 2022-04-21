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

    struct TextureDesc{
        uint32_t width       = 0;
        uint32_t height      = 0;
        VkFormat imageFormat = VK_FORMAT_UNDEFINED;
        std::vector<char> data;
    };
    void init(const TextureDesc& desc,     VulkanRenderDevice& renderDevice, bool createSampler);
    void init(const std::string& filePath, VulkanRenderDevice& renderDevice, bool createSampler);

    void destroy(VkDevice device);

    VkSampler getSampler();
    VkImageView getImageView();

private:
    static uint32_t formatToSize(VkFormat format);
    VkImage _image = nullptr;
    VkImageView _imageView = nullptr;
    VkDeviceMemory _imageMemory = nullptr;
    VkSampler _sampler = nullptr; // TODO : we really want to store the sampler in the texture ??
};



