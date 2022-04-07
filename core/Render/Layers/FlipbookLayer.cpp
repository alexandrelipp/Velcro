//
// Created by alexa on 2022-04-06.
//

#include "FlipbookLayer.h"
#include "../../events/MouseEvent.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Utils/UtilsTemplate.h"

FlipbookLayer::FlipbookLayer(VkRenderPass renderPass) {
    // create a vulkan texture for every image of the flipbook
    std::filesystem::path explosionFolder = "../../../core/Assets/Flipbooks/Explosion0";
    for (auto& file : std::filesystem::directory_iterator(explosionFolder)){
        _textures.emplace_back();
        _textures.back().init(file.path().string(), *_vrd, false);
    }

    // init vertices ssbo
    std::vector<TexVertex2> vertices;
    VK_ASSERT(FactoryModel::createTexturedSquare2(vertices), "Failed to gen vertices");

    _vertices.init(_vrd->device, _vrd->physicalDevice, false);
    _vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool, vertices.data(),
                      utils::vectorSizeByte(vertices));


    createPipelineLayout();
    createDescriptorSets();
    Factory::GraphicsPipelineProps props = {
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .shaders = {
                    .vertex = "FlipbookV",
                    .fragment = "FlipbookF"
            },
            .enableTestTest = VK_FALSE, // disable depth test! (won't really matter since we are writing at min depth anyway (0)

    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

FlipbookLayer::~FlipbookLayer() {
    for (auto& texture : _textures)
        texture.destroy(_vrd->device);
}

void FlipbookLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {

}

void FlipbookLayer::onEvent(Event& event) {
    switch (event.getType()) {
        case Event::Type::MOUSE_PRESSED:{
            MouseButtonEvent* mouseButtonEvent = (MouseButtonEvent*)&event;
            switch (mouseButtonEvent->getMouseButton()) {
                case MouseCode::ButtonLeft:
                    _animations.push_back({
                        .textureIndex = 0
                    });
                    break;
                default:
                    break;
            }
            break;
        }
        default: break;
    }

}

void FlipbookLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {

}

void FlipbookLayer::onImGuiRender() {

}

void FlipbookLayer::createPipelineLayout() {
    // create the pipeline layout
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = {
            VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
            },
            VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = (uint32_t)_textures.size(),
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = layoutBindings.size(),
            .pBindings = layoutBindings.data()
    };

    VK_CHECK(vkCreateDescriptorSetLayout(_vrd->device, &descriptorLayoutCI, nullptr, &_descriptorSetLayout));

    // create fragment push constant for camera pos
    _pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,               // must be multiple of 4 (offset into push constant block)
            .size = sizeof(uint32_t)   // must be multiple of 4
    };

    // create pipeline layout (used for uniform and push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &_pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(_vrd->device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

}

void FlipbookLayer::createDescriptorSets() {
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
                .buffer = _vpUniformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _vpUniformBuffers[i].getSize()
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

        // push back descriptor write for transform ssbos (1 buffer/ image)
        VkDescriptorBufferInfo transformsInfo = {
                .buffer = _meshTransformBuffers[i].getBuffer(),
                .offset = 0,
                .range = _meshTransformBuffers[i].getSize()
        };

        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 3,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              .pBufferInfo = &transformsInfo
                                      });

        // update the descriptor sets with the created decriptor writes
        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}

