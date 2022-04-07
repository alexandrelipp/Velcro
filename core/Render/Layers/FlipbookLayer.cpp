//
// Created by alexa on 2022-04-06.
//

#include "FlipbookLayer.h"

FlipbookLayer::FlipbookLayer(VkRenderPass renderPass) {
    std::filesystem::path explosionFolder = "../../../core/Assets/Flipbooks/Explosion0";

    for (auto& file : std::filesystem::directory_iterator(explosionFolder)){
        _textures.emplace_back();
        _textures.back().init(file.path().string(), *_vrd, false);
    }
}

FlipbookLayer::~FlipbookLayer() {
    for (auto& texture : _textures)
        texture.destroy(_vrd->device);
}

void FlipbookLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {

}

void FlipbookLayer::onEvent(Event& event) {

}

void FlipbookLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {

}

void FlipbookLayer::onImGuiRender() {

}

