//
// Created by alexa on 2022-01-31.
//

#pragma once


#include <spdlog/spdlog.h>
#include "../dep/magic_enum.hpp"


// TODO : maybe add a forceDebug flag
#ifdef NDEBUG
#define VELCO_RELEASE
#else
#define VELCRO_DEBUG
#endif

// macro bs to create unique variable name using line number
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

#define VK_CHECK(result) auto UNIQUE_NAME(s) = result; \
    if (UNIQUE_NAME(s) != VK_SUCCESS) \
        handleError(UNIQUE_NAME(s))


#define handleErrorDebug(result) do { \
      SPDLOG_ERROR("Check success failed with code {}", magic_enum::enum_name(result)); \
      throw std::runtime_error("VK FAILED");\
} while (0)

#define handleErrorRelease(result) do { \
SPDLOG_CRITICAL("Check success failed with code {}", magic_enum::enum_name(result)); \
} while (0)


#ifdef VELCRO_DEBUG
#define handleError handleErrorDebug
#else
#define handleError handleErrorRelease
#endif

#define VK_ASSERT(result, mes) if (!(result)) \
        throw std::runtime_error(mes)

#define GLM_FORCE_RADIANS
// perspective matrix ranges from -1 to 1 by default. Vulkan range is 0 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <functional>
#include <iostream>

