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

    // create fragment push constant for texture index and animation screen offset
    _pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,               // must be multiple of 4 (offset into push constant block)
            .size = sizeof(Animation)   // must be multiple of 4
    };

    // create descriptor image info for textures
    std::vector<VkDescriptorImageInfo> imagesInfo(_textures.size());
    for (uint32_t i = 0; i < imagesInfo.size(); ++i){
        imagesInfo[i] = {
                .sampler = _sampler,
                .imageView = _textures[i].getImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }

    // describe descriptors
    std::vector<Factory::Descriptor> descriptors = {
            {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .shaderStage = VK_SHADER_STAGE_VERTEX_BIT,
                    .info = std::array<VkDescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT>{
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                            VkDescriptorBufferInfo {_vertices.getBuffer(), 0, _vertices.getSize()},
                    }
            },
            {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .info = imagesInfo
            }
    };

    // create descriptors
    std::tie(_descriptorSetLayout, _pipelineLayout, _descriptorPool, _descriptorSets) =
            Factory::createDescriptorSets(_vrd, descriptors, {_pushConstantRange});

    // create graphics pipeline
    Factory::GraphicsPipelineProps props = {
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .shaders = {
                    .vertex = "FlipbookV.spv",
                    .fragment = "FlipbookF.spv"
            },
            .enableDepthTest = VK_FALSE, // disable depth test! (won't really matter since we are writing at min depth anyway (0)
            .sampleCountMSAA = _vrd->sampleCount
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

