//
// Created by alexa on 2022-03-27.
//

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>


namespace utils {

    void extractEuler(const glm::quat& q, glm::vec3& angles);

    bool decomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);

    glm::mat4 calculateModelMatrix(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation);
}


