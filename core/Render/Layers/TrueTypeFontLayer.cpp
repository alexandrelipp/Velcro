//
// Created by alexa on 2022-04-19.
//

#include "TrueTypeFontLayer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

TrueTypeFontLayer::TrueTypeFontLayer(VkRenderPass renderPass) {
    FT_Library library;
    auto error = FT_Init_FreeType(&library);
    if (error)
        SPDLOG_INFO("You suck");
    else
        SPDLOG_INFO("We did it");

}

TrueTypeFontLayer::~TrueTypeFontLayer() {

}

void TrueTypeFontLayer::update(float dt, uint32_t commandBufferIndex, const glm::mat4& pv) {

}

void TrueTypeFontLayer::onEvent(Event& event) {

}

void TrueTypeFontLayer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t commandBufferIndex) {

}

void TrueTypeFontLayer::onImGuiRender() {

}
