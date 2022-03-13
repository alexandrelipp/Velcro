#pragma once

#include <vulkan/vulkan.h>

class UniformBuffer {
public:
	UniformBuffer() = default;
	
	void init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size);
	void destroy(VkDevice device);

private:
	VkBuffer _buffer = nullptr;
	VkDeviceMemory _bufferMemory = nullptr;
};