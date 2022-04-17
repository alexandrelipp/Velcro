//
// Created by alexa on 2022-04-16.
//

// Potential optimization : might be more memory friendly to store vertices and indices in one contiguous buffer,
// as described in https://developer.nvidia.com/vulkan-memory-management

#pragma once

#include <vulkan/vulkan.h>
#include "../VulkanRenderDevice.hpp"


class VertexBuffer {
public:
    VertexBuffer() = default;
    ~VertexBuffer();

    void init(VulkanRenderDevice* vrd, void* data, uint32_t size);

    void bind(VkCommandBuffer commandBuffer);

public:
    struct VertexAttribute{
        uint32_t offset = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
    };
    /// Creates a vector of vkInputDesc with the given vertex attributes
    static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(const std::vector<VertexAttribute>& attributes);

private:
    VkBuffer _buffer = nullptr;
    VkDeviceMemory _deviceMemory = nullptr;
};

// templates are mysterious :
// https://stackoverflow.com/questions/11773960/do-template-specialisations-belong-into-the-header-or-source-file

/// Return type vulkan format associated with a given class. Must be specialized
template<typename T>
VkFormat typeToFormat(){
    // if this occurs, you must add the type down below (don't forget the inline)
    VK_ASSERT(false, "Must be specialized");
}

template<>
inline VkFormat typeToFormat<glm::vec4>(){
    return VK_FORMAT_R32G32B32A32_SFLOAT;
}

template<>
inline VkFormat typeToFormat<glm::vec3>(){
    return VK_FORMAT_R32G32B32_SFLOAT;
}

template<>
inline VkFormat typeToFormat<glm::vec2>(){
    return VK_FORMAT_R32G32_SFLOAT;
}
