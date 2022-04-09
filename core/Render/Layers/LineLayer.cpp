//
// Created by alexa on 2022-03-20.
//

#include "LineLayer.h"
#include "../Factory/FactoryVulkan.h"

LineLayer::LineLayer(VkRenderPass renderPass) : RenderLayer() {

    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers) {
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));
    }

    plane3d(glm::vec3(0.f, 0.f, -1.f), {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 10, 10, 3.f, 3.f, glm::vec4(0.7f), glm::vec4(1.f));

    auto size = _lines.size() * sizeof(VertexData);
    _pointsSSBO.init(_vrd->device, _vrd->physicalDevice, size);
    _pointsSSBO.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool, _lines.data(), size);

    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo{_mvpUniformBuffers[0].getBuffer(), 0, _mvpUniformBuffers[0].getSize()},
                            VkDescriptorBufferInfo{_mvpUniformBuffers[1].getBuffer(), 0, _mvpUniformBuffers[1].getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo{_pointsSSBO.getBuffer(), 0, _pointsSSBO.getSize()},
                            VkDescriptorBufferInfo{_pointsSSBO.getBuffer(), 0, _pointsSSBO.getSize()},
                    }
            },
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(descriptors, {}, _vrd);

    Factory::GraphicsPipelineProps props = {
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
            .shaders = {
                    .vertex = "lineV.spv",
                    .fragment = "lineF.spv"
            }
    };

    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

LineLayer::~LineLayer() {
    for (auto buffer : _mvpUniformBuffers)
        buffer.destroy(_vrd->device);

    _pointsSSBO.destroy(_vrd->device);
}

void LineLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0,
                            1, &_descriptorSets[commandBufferIndex], 0, nullptr);

    vkCmdDraw(commandBuffer, _lines.size(), 1, 0, 0);
}

void LineLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    glm::mat4 mvp = pv;
    _mvpUniformBuffers[commandBufferIndex].setData(_vrd->device, glm::value_ptr(mvp), sizeof(mvp));
}

void LineLayer::onEvent(Event& event) {}


void LineLayer::onImGuiRender() {

}

void LineLayer::line(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& c) {
    _lines.push_back( { .position = p1, .color = c } );
    _lines.push_back( { .position = p2, .color = c } );
}

void LineLayer::plane3d(const glm::vec3& o, const glm::vec3& v1, const glm::vec3& v2, int n1, int n2, float s1, float s2,
                        const glm::vec4& color, const glm::vec4& outlineColor) {
    line(o - s1 / 2.0f * v1 - s2 / 2.0f * v2, o - s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
    line(o + s1 / 2.0f * v1 - s2 / 2.0f * v2, o + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);

    line(o - s1 / 2.0f * v1 + s2 / 2.0f * v2, o + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
    line(o - s1 / 2.0f * v1 - s2 / 2.0f * v2, o + s1 / 2.0f * v1 - s2 / 2.0f * v2, outlineColor);

    for (int i = 1; i < n1; i++)
    {
        float t = ((float)i - (float)n1 / 2.0f) * s1 / (float)n1;
        const glm::vec3 o1 = o + t * v1;
        line(o1 - s2 / 2.0f * v2, o1 + s2 / 2.0f * v2, color);
    }

    for (int i = 1; i < n2; i++)
    {
        const float t = ((float)i - (float)n2 / 2.0f) * s2 / (float)n2;
        const glm::vec3 o2 = o + t * v2;
        line(o2 - s1 / 2.0f * v1, o2 + s1 / 2.0f * v1, color);
    }
}


//////////////////////// PRIVATE METHODS ///////////////////////////////////
