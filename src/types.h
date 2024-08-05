#pragma once

#include <cstdint>
#include <glm/glm.hpp>


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

union GXRgb {
    u16 color;
    struct {
        u16 r : 5;
        u16 g : 5;
        u16 b : 5;
        u16 : 1;
    };

    GXRgb() = default;
    GXRgb(u16 color) : color(color) {}
    GXRgb(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}
    GXRgb(const glm::vec3& vec) {
        r = vec.r * 31.0f;
        g = vec.g * 31.0f;
        b = vec.b * 31.0f;
    }

    glm::vec3 toVec3() const {
        return glm::vec3(r / 31.0f, g / 31.0f, b / 31.0f);
    }

    operator glm::vec3() const {
        return toVec3();
    }
};
