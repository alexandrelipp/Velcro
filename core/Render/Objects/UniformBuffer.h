#pragma once

#include <vulkan/vulkan.h>

class UniformBuffer {
public:
	UniformBuffer() = default;
	
	void init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size);
	void destroy(VkDevice device);

	uint32_t getSize();
	VkBuffer getBuffer();

	bool setData(VkDevice device, void* data, uint32_t size);

private:
	VkBuffer _buffer = nullptr;
	VkDeviceMemory _bufferMemory = nullptr;
	uint32_t _size{};
};