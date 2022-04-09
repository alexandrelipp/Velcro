//
// Created by alexa on 2022-03-15.
//

#include "Texture.h"

#include "../Factory/FactoryVulkan.h"
#include "../../Utils/UtilsFile.h"
#include "../../Utils/UtilsVulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stbi_image.h>

void Texture::init(const std::string& filePath, VulkanRenderDevice& renderDevice, bool createSampler) {
    // make sure given file exists
    VK_ASSERT(utils::fileExists(filePath), "Texture file does not exist");

    // load pixels data using the stb library
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, 4);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VK_ASSERT(pixels != nullptr, "failed to load texture image!");

    // create staging buffer for transfer
    auto stagingBuffer = Factory::createBuffer(renderDevice.device, renderDevice.physicalDevice, imageSize,
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    // copy pixels to staging buffer
    void* dst = nullptr;
    VK_CHECK(vkMapMemory(renderDevice.device, stagingBuffer.second, 0, imageSize, 0, &dst));
    memcpy(dst, pixels, imageSize);
    vkUnmapMemory(renderDevice.device, stagingBuffer.second);

    // free pixels after copy
    stbi_image_free(pixels);

    // format of the image (maybe a function to pick the best one ?)
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    auto image = Factory::createImage(renderDevice.device, renderDevice.physicalDevice, texWidth, texHeight, imageFormat,
                                      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    _image = image.first;
    _imageMemory = image.second;

    // transition image layout UNDEFINED -> DST_OPTIMAL
    utils::transitionImageLayout(renderDevice.device, renderDevice.graphicsQueue, renderDevice.commandPool, _image, imageFormat,
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy staging buffer -> image. Note : this should probably be extracted in a utils generic function
    utils::executeOnQueueSync(renderDevice.graphicsQueue, renderDevice.device, renderDevice.commandPool,
                              [&, this](VkCommandBuffer commandBuffer){
        VkBufferImageCopy imageRegion = {
                .bufferOffset = 0,
                .bufferRowLength = 0,     // would matter if data was not tightly pacted
                .bufferImageHeight = 0,   // would matter if data was not tightly pacted
                .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                },
                .imageOffset = {
                        .x = 0,
                        .y = 0,
                        .z = 0,
                },
                .imageExtent = {
                        .width = (uint32_t)texHeight,
                        .height = (uint32_t)texHeight,
                        .depth = 1,
                }
        };
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.first, _image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
    });


    // transition from DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    utils::transitionImageLayout(renderDevice.device, renderDevice.graphicsQueue, renderDevice.commandPool, _image, imageFormat,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // delete the staging buffer after the transfer
    vkFreeMemory(renderDevice.device, stagingBuffer.second, nullptr);
    vkDestroyBuffer(renderDevice.device, stagingBuffer.first, nullptr);

    _imageView = Factory::createImageView(renderDevice.device, _image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    // done initializing if not creating a sampler
    if (!createSampler)
        return;

    VkSamplerCreateInfo samplerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .flags = 0u,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,    // interpolation mode between MIP
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.f,                              // Lod bias for mip level
            .anisotropyEnable = VK_TRUE,                    // enable Ansiotropy, furthest things looks better
            .maxAnisotropy = 16.f,                          // anisotropy sample level
            .minLod = 0.f,                                  // min level of detail to pick mip level
            .maxLod = 0.f,                                  // max level of dtail to pick mip level
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, // only applied when repeat mode is clamp to border
            .unnormalizedCoordinates = VK_FALSE,

    };

    VK_CHECK(vkCreateSampler(renderDevice.device, &samplerCreateInfo, nullptr, &_sampler));

}

void Texture::destroy(VkDevice device) {
    if (_sampler != nullptr)
        vkDestroySampler(device, _sampler, nullptr);
    vkDestroyImageView(device, _imageView, nullptr);
    vkFreeMemory(device, _imageMemory, nullptr);
    vkDestroyImage(device, _image, nullptr);

    _sampler = nullptr;
    _imageView = nullptr;
    _imageMemory = nullptr;
    _image = nullptr;
}

VkSampler Texture::getSampler() {
    VK_ASSERT(_sampler != nullptr, "Sampler is null ");
    return _sampler;
}

VkImageView Texture::getImageView() {
    return _imageView;
}
