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
        u16 _ : 1;
    };

    GXRgb() = default;
    GXRgb(u16 color) : color(color) {}
    GXRgb(u8 r, u8 g, u8 b) : r(r), g(g), b(b), _(0) {}
    GXRgb(const glm::vec3& vec) {
        r = (u16)(vec.r * 31.0f);
        g = (u16)(vec.g * 31.0f);
        b = (u16)(vec.b * 31.0f);
        _ = 0;
    }

    glm::vec3 toVec3() const {
        return glm::vec3(r / 31.0f, g / 31.0f, b / 31.0f);
    }

    operator glm::vec3() const {
        return toVec3();
    }

    bool operator==(const GXRgb& other) const {
        return color == other.color;
    }

    bool operator!=(const GXRgb& other) const {
        return color != other.color;
    }

    static auto fromRGB(u8 r, u8 g, u8 b) {
        return GXRgb(r >> 3, g >> 3, b >> 3);
    }
};

union GXRgba {
    u16 color;
    struct {
        u16 r : 5;
        u16 g : 5;
        u16 b : 5;
        u16 a : 1;
    };

    GXRgba() = default;
    GXRgba(u16 color) : color(color) {}
    GXRgba(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) {}
    GXRgba(const glm::vec4& vec) {
        r = vec.r * 31.0f;
        g = vec.g * 31.0f;
        b = vec.b * 31.0f;
        a = vec.a > 0.5f;
    }

    glm::vec4 toVec4() const {
        return glm::vec4(r / 31.0f, g / 31.0f, b / 31.0f, a);
    }

    operator glm::vec4() const {
        return toVec4();
    }

    bool operator==(const GXRgba& other) const {
        return color == other.color;
    }

    bool operator!=(const GXRgba& other) const {
        return color != other.color;
    }

    u8 r8() const {
        return (r << 3) | (r >> 2);
    }

    u8 g8() const {
        return (g << 3) | (g >> 2);
    }

    u8 b8() const {
        return (b << 3) | (b >> 2);
    }

    u8 a8() const {
        return a ? 0xFF : 0;
    }

    static auto fromRGBA(u8 r, u8 g, u8 b, u8 a) {
        return GXRgba(r >> 3, g >> 3, b >> 3, a > 0x80);
    }
};

enum class TextureFormat : u8 {
    None = 0,
    A3I5, // 3 bits alpha, 5-bit palette index
    Palette4, // 2-bit palette index (4 colors)
    Palette16, // 4-bit palette index (16 colors)
    Palette256, // 8-bit palette index (256 colors)
    Comp4x4, // 4x4 compression
    A5I3, // 5 bits alpha, 3-bit palette index
    Direct, // 16-bit R5G5B5A1 (RGBA5551), no palette

    Count,
};

enum class TextureRepeat {
    None,
    S,
    T,
    ST,
};

typedef TextureRepeat TextureFlip;
