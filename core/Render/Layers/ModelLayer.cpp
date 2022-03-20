//
// Created by alexa on 2022-03-20.
//

#include "ModelLayer.h"
#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

ModelLayer::ModelLayer(VkRenderPass renderPass) : RenderLayer() {
    // init the uniform buffers
    for (auto& buffer : _mvpUniformBuffers)
        buffer.init(_vrd->device, _vrd->physicalDevice, sizeof(mvp));

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
    _texture.init("../../../core/Assets/Models/duck/textures/Duck_baseColor.png", _vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool);
    createPipelineLayout();
    createDescriptorSets();
    ShaderFiles files = {
            .vertex = "vert.spv",
            .fragment = "frag.spv"
    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, files);
}

ModelLayer::~ModelLayer() {

}

void ModelLayer::update(float dt, uint32_t currentImage) {
    static float time = 0.f;
    time += dt;
    float aspectRatio = _swapchainExtent.width/(float)_swapchainExtent.height;
    glm::mat4 v = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 p = glm::perspective(45.f, aspectRatio, 0.1f, 1000.f);
    glm::mat4 m = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.5f, -1.5f)) * glm::rotate(glm::mat4(1.f), glm::pi<float>(),
                                                                                       glm::vec3(1, 0, 0)),
            time,
            glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 mvp = p  * v * m;
    VK_ASSERT(_mvpUniformBuffers[currentImage].setData(_vrd->device, glm::value_ptr(mvp), sizeof(mvp)), "Failed to dat");
}


void ModelLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) {
    // bind pipeline and render
    vkCmdBindPipeline(_vrd->commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(_vrd->commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                            0, 1, &_descriptorSets[currentImage], 0, nullptr);

    vkCmdDraw(_vrd->commandBuffers[currentImage], _indexCount, 1, 0, 0);
}


//////////////// PRIVATE METHODS /////////////////////////

void ModelLayer::createPipelineLayout() {
    // create the pipeline layout
    std::array<VkDescriptorSetLayoutBinding, 4> layoutBindings = {
            VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = layoutBindings.size(),
            .pBindings = layoutBindings.data()
    };

    VK_CHECK(vkCreateDescriptorSetLayout(_vrd->device, &descriptorLayoutCI, nullptr, &_descriptorSetLayout));


    // create pipeline layout (used for uniform and push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VK_CHECK(vkCreatePipelineLayout(_vrd->device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void ModelLayer::createDescriptorSets() {
    // Inadequate descriptor pools are a good example of a problem that the validation layers will not catch:
    // As of Vulkan 1.1, vkAllocateDescriptorSets may fail with the error code VK_ERROR_POOL_OUT_OF_MEMORY
    // if the pool is not sufficiently large, but the driver may also try to solve the problem internally.
    // This means that sometimes (depending on hardware, pool size and allocation size) the driver will let
    // us get away with an allocation that exceeds the limits of our descriptor pool.
    // Other times, vkAllocateDescriptorSets will fail and return VK_ERROR_POOL_OUT_OF_MEMORY.
    // This can be particularly frustrating if the allocation succeeds on some machines, but fails on others.
    _descriptorPool = Factory::createDescriptorPool(_vrd->device, FB_COUNT, 1, 2, 0);

    std::array<VkDescriptorSetLayout, FB_COUNT> layouts = {_descriptorSetLayout, _descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_vrd->device, &descriptorSetAI, _descriptorSets.data()));

    for (size_t i = 0; i < _descriptorSets.size(); ++i) {
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _mvpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _mvpUniformBuffers[i].getSize()
        };

        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 0,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              .pBufferInfo = &bufferInfo
                                      });

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

        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

}