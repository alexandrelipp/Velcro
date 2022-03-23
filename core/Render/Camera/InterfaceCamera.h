#pragma once

#include <glm/glm.hpp>

// TODO : include projection matrix in interface ?
class InterfaceCamera {
public:
    virtual ~InterfaceCamera() = default;
    virtual glm::mat4 getViewMatrix() const = 0;
    virtual glm::vec3 getPosition() const = 0;
};