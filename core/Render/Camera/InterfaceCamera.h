#pragma once

#include <glm/glm.hpp>
#include "../../events/Event.h"

// TODO : include projection matrix in interface ?
class InterfaceCamera {
public:
    virtual ~InterfaceCamera() = default;

    virtual void update(float deltaTime) = 0;
    virtual void onEvent(Event& event) = 0;

    virtual glm::mat4* getPVMatrix() = 0;
    virtual glm::vec3* getPosition() = 0;
};