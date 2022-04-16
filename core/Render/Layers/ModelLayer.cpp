//
// Created by alexa on 2022-03-20.
//

#include "ModelLayer.h"
#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Application.h"
#include "../../Utils/UtilsTemplate.h"

ModelLayer::ModelLayer(VkRenderPass renderPass) : RenderLayer() {
    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    // create duck model
    std::vector<TexVertex> vertices;
    std::vector<uint32_t> indices;
    VK_ASSERT(FactoryModel::createDuckModel(vertices, indices), "Failed to create mesh");
    _indexCount = indices.size();

    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> inputDescriptions(2);
    inputDescriptions[0].location = 0,
    inputDescriptions[0].binding = 0,
    inputDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT,
    inputDescriptions[0].offset = offsetof(Vertex, position);

    inputDescriptions[0].location = 1,
    inputDescriptions[0].binding = 0,
    inputDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT,
    inputDescriptions[0].offset = offsetof(Vertex, uv);


    // init the vertices ssbo
    _vertices.init(_vrd, utils::vectorSizeByte(vertices), vertices.data());

    // init the indices ssbo
    _indices.init(_vrd, utils::vectorSizeByte(indices), indices.data());

    // init the statue texture
    _texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", *_vrd, true);
    createDescriptors();
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "vert.spv",
                    .fragment = "frag.spv"
            },
            .sampleCountMSAA = _vrd->sampleCount
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

ModelLayer::~ModelLayer() {
    // destroy the buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
    _indices.destroy(_vrd->device);

    _texture.destroy(_vrd->device);
}

void ModelLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    static float time = 0.f;
    time += dt;
    glm::mat4 m = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.5f, -1.5f)) * glm::rotate(glm::mat4(1.f), glm::pi<float>(),
                                                                                       glm::vec3(1, 0, 0)),
            time,
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 mvp = pv * m;
    VK_ASSERT(_mvpUniformBuffers[commandBufferIndex].setData(_vrd->device, glm::value_ptr(mvp), sizeof(mvp)), "Failed to dat");
}

void ModelLayer::onEvent(Event& event) {}


void ModelLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    // bind pipeline and render
    bindPipelineAndDS(commandBuffer, commandBufferIndex);
    vkCmdDraw(_vrd->commandBuffers[commandBufferIndex], _indexCount, 1, 0, 0);
}

void ModelLayer::onImGuiRender() {

}


//////////////// PRIVATE METHODS /////////////////////////

void ModelLayer::createDescriptors() {
    // describe
    std::vector<Factory::Descriptor> descriptors = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                    VkDescriptorBufferInfo {_mvpUniformBuffers[0].getBuffer(), 0, _mvpUniformBuffers[0].getSize()},
                    VkDescriptorBufferInfo {_mvpUniformBuffers[1].getBuffer(), 0, _mvpUniformBuffers[1].getSize()},
                }
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                        VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                        VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                }
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                        VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                        VkDescriptorBufferInfo {_indices.getBuffer(), 0, _indices.getSize()},
                }
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .info = std::vector<VkDescriptorImageInfo>{
                    VkDescriptorImageInfo{
                            .sampler = _texture.getSampler(),
                            .imageView = _texture.getImageView(),
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    }
                }
            }
    };

    /// create descriptor sets
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {});
}