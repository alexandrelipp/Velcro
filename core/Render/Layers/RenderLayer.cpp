//
// Created by alexa on 2022-03-20.
//

#include "RenderLayer.h"

#include "../../Application.h"

RenderLayer::RenderLayer() {
    if (_vrd == nullptr || _vrd->device == nullptr){
        Renderer* renderer = Application::getApp()->getRenderer();
        _vrd = renderer->getRenderDevice();
        _swapchainExtent = renderer->getSwapchainExtent();
    }
}

RenderLayer::~RenderLayer() {

}
