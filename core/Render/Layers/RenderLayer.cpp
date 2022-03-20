//
// Created by alexa on 2022-03-20.
//

#include "RenderLayer.h"

#include "../../Application.h"

RenderLayer::~RenderLayer() {
    if (_device == nullptr){
        Renderer* renderer = Application::getApp()->getRenderer();
        _device = renderer->getDevice();
        _swapchainExtent = renderer->getSwapchainExtent();
    }
}
