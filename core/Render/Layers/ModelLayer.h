//
// Created by alexa on 2022-03-20.
//

#pragma once

#include "RenderLayer.h"


class ModelLayer : public RenderLayer{
public:
    ModelLayer();
    virtual ~ModelLayer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentImage) override;

private:

};



