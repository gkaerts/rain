#pragma once

#include "common/math/math.hpp"
#include <cstring>

#define RN_MAP_SHADER_TYPE(underlying, suffix, elementCount)                \
struct underlying##suffix {                                                 \
    underlying##suffix() { std::memset(values, 0, sizeof(values)); }        \
    underlying##suffix(const underlying##suffix& rhs) {                     \
        std::memcpy(this, &rhs, sizeof(values));                            \
    }                                                                       \
    underlying##suffix(underlying##suffix&& rhs) {                          \
        std::memcpy(this, &rhs, sizeof(values));                            \
    }                                                                       \
    underlying##suffix(const rn::underlying##suffix& value) {               \
        std::memcpy(this, &value, sizeof(values));                          \
    }                                                                       \
    underlying##suffix(const underlying(&arr)[elementCount]) {              \
        std::memcpy(this, arr, sizeof(values));                             \
    }                                                                       \
    underlying##suffix& operator=(const underlying##suffix& value) {        \
        std::memcpy(this, &value, sizeof(values));                          \
    }                                                                       \
    underlying##suffix& operator=(const rn::underlying##suffix& value) {    \
        std::memcpy(this, &value, sizeof(values));                          \
    }                                                                       \
    underlying##suffix& operator=(const underlying(&arr)[elementCount]) {   \
        std::memcpy(this, arr, sizeof(values));                             \
    }                                                                       \
    underlying values[elementCount];                                        \
};

RN_MAP_SHADER_TYPE(float, 1, 1)
RN_MAP_SHADER_TYPE(float, 2, 2)
RN_MAP_SHADER_TYPE(float, 3, 3)
RN_MAP_SHADER_TYPE(float, 4, 4)
RN_MAP_SHADER_TYPE(float, 1x1, 1)
RN_MAP_SHADER_TYPE(float, 1x2, 2)
RN_MAP_SHADER_TYPE(float, 1x3, 3)
RN_MAP_SHADER_TYPE(float, 1x4, 4)
RN_MAP_SHADER_TYPE(float, 2x1, 2)
RN_MAP_SHADER_TYPE(float, 2x2, 4)
RN_MAP_SHADER_TYPE(float, 2x3, 6)
RN_MAP_SHADER_TYPE(float, 2x4, 8)
RN_MAP_SHADER_TYPE(float, 3x1, 3)
RN_MAP_SHADER_TYPE(float, 3x2, 6)
RN_MAP_SHADER_TYPE(float, 3x3, 9)
RN_MAP_SHADER_TYPE(float, 3x4, 12)
RN_MAP_SHADER_TYPE(float, 4x1, 4)
RN_MAP_SHADER_TYPE(float, 4x2, 8)
RN_MAP_SHADER_TYPE(float, 4x3, 12)
RN_MAP_SHADER_TYPE(float, 4x4, 16)

RN_MAP_SHADER_TYPE(int, 1, 1)
RN_MAP_SHADER_TYPE(int, 2, 2)
RN_MAP_SHADER_TYPE(int, 3, 3)
RN_MAP_SHADER_TYPE(int, 4, 4)

using uint = uint32_t;
RN_MAP_SHADER_TYPE(uint, 1, 1)
RN_MAP_SHADER_TYPE(uint, 2, 2)
RN_MAP_SHADER_TYPE(uint, 3, 3)
RN_MAP_SHADER_TYPE(uint, 4, 4)