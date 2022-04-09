//
// Created by alexa on 2022-04-06.
//

#include "FlipbookLayer.h"
#include "../../events/MouseEvent.h"

#include "../Factory/FactoryVulkan.h"
#include "../Factory/FactoryModel.h"

#include "../../Utils/UtilsTemplate.h"
#include "../../Application.h"

FlipbookLayer::FlipbookLayer(VkRenderPass renderPass) {
    // create a vulkan texture for every image of the flipbook
    std::filesystem::path explosionFolder = "../../../core/Assets/Flipbooks/Explosion0";
    for (auto& file : std::filesystem::directory_iterator(explosionFolder)){
        _textures.emplace_back();
        _textures.back().init(file.path().string(), *_vrd, false);
    }

    // create sampler
    VkSamplerCreateInfo samplerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .flags = 0u,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,    // interpolation mode between MIP
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.f,                              // Lod bias for mip level
            .anisotropyEnable = VK_FALSE,                    // disable anisotropy, not necessary
            .maxAnisotropy = 16.f,                          // anisotropy sample level
            .minLod = 0.f,                                  // min level of detail to pick mip level
            .maxLod = 0.f,                                  // max level of dtail to pick mip level
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, // only applied when repeat mode is clamp to border
            .unnormalizedCoordinates = VK_FALSE,

    };
    VK_CHECK(vkCreateSampler(_vrd->device, &samplerCreateInfo, nullptr, &_sampler));


    // init vertices ssbo
    std::vector<TexVertex2> vertices;
    VK_ASSERT(FactoryModel::createTexturedSquare2(vertices), "Failed to gen vertices");

    uint32_t verticesSize = utils::vectorSizeByte(vertices);
    _vertices.init(_vrd->device, _vrd->physicalDevice, verticesSize);
    _vertices.setData(_vrd->device, _vrd->physicalDevice, _vrd->graphicsQueue, _vrd->commandPool, vertices.data(),
                      verticesSize);

    // create graphics pipeline
    createPipelineLayout();
    createDescriptorSets();
    Factory::GraphicsPipelineProps props = {
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .shaders = {
                    .vertex = "FlipbookV.spv",
                    .fragment = "FlipbookF.spv"
            },
            .enableDepthTest = VK_FALSE, // disable depth test! (won't really matter since we are writing at min depth anyway (0)

    };
    _graphicsPipeline = Factory::createGraphicsPipeline(_vrd->device, _swapchainExtent, renderPass, _pipelineLayout, props);
}

FlipbookLayer::~FlipbookLayer() {
    vkDestroySampler(_vrd->device, _sampler, nullptr);
    for (auto& texture : _textures)
        texture.destroy(_vrd->device);
    _vertices.destroy(_vrd->device);
}

void FlipbookLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {
    if (!_animation.has_value())
        return;

    // increase texture index by 1 each frame. Not that good, animation speed depends on frame rate
    _animation.value().textureIndex += 1;

    if (_animation.value().textureIndex == _textures.size())
        _animation = std::nullopt;
}

void FlipbookLayer::onEvent(Event& event) {
    switch (event.getType()) {
        case Event::Type::MOUSE_PRESSED:{
            MouseButtonEvent* mouseButtonEvent = (MouseButtonEvent*)&event;
            switch (mouseButtonEvent->getMouseButton()) {
                case MouseCode::ButtonLeft:{
                    // calculate screen coordinate offset : http://vulkano.rs/guide/vertex-input
                    glm::vec2 pos = Application::getApp()->getMousePos();
                    glm::vec2 size = Application::getApp()->getWindowSize();
                    glm::vec2 vulkanScreenCoordinate = pos/size * 2.f - 1.f;

                    _animation = {
                        .textureIndex = 0,
                        .offset =  vulkanScreenCoordinate
                    };
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default: break;
    }

}

void FlipbookLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {
    if (!_animation.has_value())
        return;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[commandBufferIndex], 0, nullptr);
    vkCmdPushConstants(commandBuffer, _pipelineLayout, _pushConstantRange.stageFlags, _pushConstantRange.offset, _pushConstantRange.size, &_animation.value());
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void FlipbookLayer::onImGuiRender() {}

void FlipbookLayer::createPipelineLayout() {
    // create the pipeline layout
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = {
            VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
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

    // create fragment push constant for texture index and animation screen offset
    _pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,               // must be multiple of 4 (offset into push constant block)
            .size = sizeof(Animation)   // must be multiple of 4
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
    _descriptorPool = Factory::createDescriptorPool(_vrd->device, MAX_FRAMES_IN_FLIGHT, _textures.size(), 0, 1);

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = {_descriptorSetLayout, _descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = _descriptorPool,
            .descriptorSetCount = (uint32_t)layouts.size(),
            .pSetLayouts = layouts.data()
    };

    VK_CHECK(vkAllocateDescriptorSets(_vrd->device, &descriptorSetAI, _descriptorSets.data()));

    std::vector<VkDescriptorImageInfo> textureDescriptors(_textures.size());
    for (int i = 0; i < _textures.size(); ++i){
        textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureDescriptors[i].imageView = _textures[i].getImageView();
        textureDescriptors[i].sampler = _sampler;
    }

    for (size_t i = 0; i < _descriptorSets.size(); ++i) {
        // create write descriptor set for each descriptor set
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        // push back descriptor write for UBO (1 buffer/image)
        VkDescriptorBufferInfo bufferInfo = {
                .buffer = _vertices.getBuffer(),
                .offset = 0,
                .range = _vertices.getSize()
        };
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 0,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              .pBufferInfo = &bufferInfo
                                      });

        // push back descriptors for textures
        writeDescriptorSets.push_back({
                                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                              .dstSet = _descriptorSets[i],
                                              .dstBinding = 1,
                                              .dstArrayElement = 0,
                                              .descriptorCount = (uint32_t)textureDescriptors.size(),
                                              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              .pImageInfo = textureDescriptors.data()
                                      });

        // update the descriptor sets with the created decriptor writes
        vkUpdateDescriptorSets(_vrd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
    //std::tie(_descriptorSetLayout, _descriptorPool, _descriptorSets) = Factory::createDescriptorSets();
}

