#include "UniformBuffer.h"

#include "../Factory/FactoryVulkan.h"

void UniformBuffer::init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size){
    _size = size;
    auto buffer = Factory::createBuffer(device, physicalDevice, size,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	_buffer = buffer.first;
	_bufferMemory = buffer.second;
}

void UniformBuffer::destroy(VkDevice device){
	vkDestroyBuffer(device, _buffer, nullptr);
	vkFreeMemory(device, _bufferMemory, nullptr);

	_bufferMemory = nullptr;
	_buffer = nullptr;
}

uint32_t UniformBuffer::getSize() {
    return _size;
}

VkBuffer UniformBuffer::getBuffer() {
    return _buffer;
}

bool UniformBuffer::setData(VkDevice device, void* data, uint32_t size) {
	if (size > _size)
		return false;
	void* pData = nullptr;
	VK_CHECK(vkMapMemory(device, _bufferMemory, 0, size, 0, &pData));
	memcpy(pData, data, size);

	vkUnmapMemory(device, _bufferMemory);

	return true;
}


