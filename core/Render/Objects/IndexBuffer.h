//
// Created by alexa on 2022-04-17.
//

#pragma once


#include "../VulkanRenderDevice.hpp"

class IndexBuffer {
public:
    IndexBuffer() = default;
    ~IndexBuffer();

    void init(VulkanRenderDevice* vrd, VkIndexType indexType, void* data, uint32_t indexCount);

    void bind(VkCommandBuffer commandBuffer);

    [[nodiscard]] uint32_t getIndexCount() const;

private:
    /// Returns number of bytes for given index type
    static constexpr uint32_t indexTypeSize(VkIndexType type);

private:
    VkBuffer        _buffer       = nullptr;
    VkDeviceMemory  _deviceMemory = nullptr;
    VkIndexType     _indexType    = VK_INDEX_TYPE_NONE_KHR;
    uint32_t        _indexCount   = 0;
};