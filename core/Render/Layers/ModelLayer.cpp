//
// Created by alexa on 2022-03-20.
//

#include "ModelLayer.h"
#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Application.h"

ModelLayer::ModelLayer(VkRenderPass renderPass) : RenderLayer() {
    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(glm::mat4));

    // create duck model
    std::vector<TexVertex> vertices;
    std::vector<uint32_t> indices;
    VK_ASSERT(FactoryModel::createDuckModel(vertices, indices), "Failed to create mesh");
    _indexCount = indices.size();

    // init the vertices ssbo
    _vertices.init(_vrd->device, _vrd->physicalDevice, vertices.size() * sizeof(vertices[0]));
    VK_ASSERT(_vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                                vertices.data(), vertices.size() * sizeof(vertices[0])), "set data failed");

    // init the indices ssbo
    _indices.init(_vrd->device, _vrd->physicalDevice, sizeof(indices[0]) * indices.size());
    VK_ASSERT(_indices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool,
                               indices.data(), indices.size() * sizeof(indices[0])), "set data failed");

    // init the statue texture
    _texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", *_vrd, true);
    createPipelineLayout();
    //createDescriptorSets();
    Factory::GraphicsPipelineProps props = {
            .shaders =  {
                    .vertex = "vert.spv",
                    .fragment = "frag.spv"
            }
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
    vkCmdBindPipeline(_vrd->commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(_vrd->commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[commandBufferIndex], 0, nullptr);

    vkCmdDraw(_vrd->commandBuffers[commandBufferIndex], _indexCount, 1, 0, 0);
}

void ModelLayer::onImGuiRender() {

}



//////////////// PRIVATE METHODS /////////////////////////

void ModelLayer::createPipelineLayout() {
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
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(descriptors, {}, _vrd);
//    // create the pipeline layout
//    std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings = {
//            VkDescriptorSetLayoutBinding{
//                    .binding = 0,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                    .descriptorCount = 1,
//                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
//            },
//            VkDescriptorSetLayoutBinding{
//                    .binding = 1,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//                    .descriptorCount = 1,
//                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
//            },
//            VkDescriptorSetLayoutBinding{
//                    .binding = 2,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//                    .descriptorCount = 1,
//                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
//            },
//            VkDescriptorSetLayoutBinding{
//                    .binding = 3,
//                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//                    .descriptorCount = 1,
//                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
//            }
//    };
//
//    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
//            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//            .bindingCount = layoutBindings.size(),
//            .pBindings = layoutBindings.data()
//    };
//
//    VK_CHECK(vkCreateDescriptorSetLayout(_vrd->device, &descriptorLayoutCI, nullptr, &_descriptorSetLayout));
//
//
//    // create pipeline layout (used for uniform and push constants)
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 1;
//    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
//    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
//
//    VK_CHECK(vkCreatePipelineLayout(_vrd->device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void ModelLayer::createDescriptorSets() {

    // Inadequate descriptor pools are a good example of a problem that the validation layers will not catch:
    // As of Vulkan 1.1, vkAllocateDescriptorSets may fail with the error code VK_ERROR_POOL_OUT_OF_MEMORY
    // if the pool is not sufficiently large, but the driver may also try to solve the problem internally.
    // This means that sometimes (depending on hardware, pool size and allocation size) the driver will let
    // us get away with an allocation that exceeds the limits of our descriptor pool.
    // Other times, vkAllocateDescriptorSets will fail and return VK_ERROR_POOL_OUT_OF_MEMORY.
    // This can be particularly frustrating if the allocation succeeds on some machines, but fails on others.
    _descriptorPool = Factory::createDescriptorPool(_vrd->device, MAX_FRAMES_IN_FLIGHT, 1, 2, 1);

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = {_descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_vrd->device, &descriptorSetAI, _descriptorSets.data()));

    for (size_t i = 0; i < _descriptorSets.size(); ++i) {
        // create write descriptor set for each descriptor set
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        // push back descriptor write for UBO (1 buffer/image)
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _mvpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _mvpUniformBuffers[i].getSize()
        };
        writeDescriptorSets.push_back({
                                  .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = _descriptorSets[i],
                                  .dstBinding = 0,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  .pBufferInfo = &bufferInfo
                                      });

        // push back descriptor write for vertices SSBO (1 buffer)
        VkDescriptorBufferInfo verticesInfo = {
                .buffer = _vertices.getBuffer(),
                .offset = 0,
                .range = _vertices.getSize()
        };
        writeDescriptorSets.push_back({
                                  .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = _descriptorSets[i],
                                  .dstBinding = 1,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  .pBufferInfo = &verticesInfo
                                      });

        // push back descriptor write for indices SSBO (1 buffer)
        VkDescriptorBufferInfo indicesInfo = {
                .buffer = _indices.getBuffer(),
                .offset = 0,
                .range = _indices.getSize()
        };
        writeDescriptorSets.push_back({
                                  .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = _descriptorSets[i],
                                  .dstBinding = 2,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  .pBufferInfo = &indicesInfo
                                      });

        // push back descriptor write for image/sampler
        VkDescriptorImageInfo imageInfo = {
                .sampler = _texture.getSampler(),
                .imageView = _texture.getImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writeDescriptorSets.push_back({
                                  .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = _descriptorSets[i],
                                  .dstBinding = 3,
                                  .dstArrayElement = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = &imageInfo
                                      });

        // update the descriptor sets with the created decriptor writes
        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

}