#include "spl_resource.h"

#include <ranges>


namespace {

template<typename T> requires std::is_trivially_copyable_v<T>
constexpr std::span<const T> asSpan(const std::vector<u8>& data) {
    return { reinterpret_cast<const T*>(data.data()), data.size() / sizeof(T) };
}

std::vector<u8> extractPaletteGXRgba(const u8* rgba, size_t pixelCount, size_t maxColors) {
    std::vector<GXRgba> palette;

    for (size_t i = 0; i < pixelCount; i++) {
        const u8 r = rgba[i * 4 + 0];
        const u8 g = rgba[i * 4 + 1];
        const u8 b = rgba[i * 4 + 2];
        const u8 a = rgba[i * 4 + 3];

        const auto color = GXRgba::fromRGBA(r, g, b, a);

        if (std::ranges::find(palette, color) == palette.end()) {
            if (palette.size() < maxColors) {
                palette.push_back(color);
            } else {
                throw std::runtime_error("Too many colors in palette");
            }
        }
    }

    return { (u8*)palette.data(), (u8*)(palette.data() + palette.size()) };
}

std::vector<u8> extractPaletteGXRgb(const u8* rgba, size_t pixelCount, size_t maxColors) {
    std::vector<GXRgb> palette;

    for (size_t i = 0; i < pixelCount; i++) {
        const u8 r = rgba[i * 4 + 0];
        const u8 g = rgba[i * 4 + 1];
        const u8 b = rgba[i * 4 + 2];

        const auto color = GXRgb::fromRGB(r, g, b);

        if (std::ranges::find(palette, color) == palette.end()) {
            if (palette.size() < maxColors) {
                palette.push_back(color);
            } else {
                throw std::runtime_error("Too many colors in palette");
            }
        }
    }

    return { (u8*)palette.data(), (u8*)(palette.data() + palette.size()) };
}

size_t findPaletteIndexGXRgb(GXRgb color, std::span<const GXRgb> palette) {
    const auto it = std::ranges::find(palette, color);
    if (it != palette.end()) {
        return std::distance(palette.begin(), it);
    }

    return -1;
}

size_t findPaletteIndexGXRgba(GXRgba color, std::span<const GXRgba> palette) {
    const auto it = std::ranges::find(palette, color);
    if (it != palette.end()) {
        return std::distance(palette.begin(), it);
    }

    return -1;
}

void convertToA3I5(const u8* rgba, PixelA3I5* out, size_t pixelCount, std::span<const GXRgb> palette) {
    for (size_t i = 0; i < pixelCount; i++) {
        const auto color = GXRgb::fromRGB(
            rgba[i * 4 + 0],
            rgba[i * 4 + 1],
            rgba[i * 4 + 2]
        );

        const u8 alpha = rgba[i * 4 + 3] >> 5;
        const auto index = findPaletteIndexGXRgb(color, palette);

        if (index != -1) {
            out[i] = { .color = (u8)index, .alpha = alpha };
        }
    }
}

void convertToA5I3(const u8* rgba, PixelA5I3* out, size_t pixelCount, std::span<const GXRgb> palette) {
    for (size_t i = 0; i < pixelCount; i++) {
        const auto color = GXRgb::fromRGB(
            rgba[i * 4 + 0],
            rgba[i * 4 + 1],
            rgba[i * 4 + 2]
        );

        const u8 alpha = rgba[i * 4 + 3] >> 3;
        const auto index = findPaletteIndexGXRgb(color, palette);

        if (index != -1) {
            out[i] = { .color = (u8)index, .alpha = alpha };
        }
    }
}

void convertToPalette4(const uint8_t* rgba, u8* out, size_t pixelCount, std::span<const GXRgba> palette) {
    for (size_t i = 0; i < pixelCount; i += 4) {
        uint8_t byte = 0;
        for (int j = 0; j < 4 && (i + j) < pixelCount; ++j) {
            const u8 r = rgba[(i + j) * 4 + 0];
            const u8 g = rgba[(i + j) * 4 + 1];
            const u8 b = rgba[(i + j) * 4 + 2];
            const u8 a = rgba[(i + j) * 4 + 3];

            const auto index = findPaletteIndexGXRgba(GXRgba::fromRGBA(r, g, b, a), palette);
            if (index != -1) {
                byte |= (index & 0x03) << (j * 2);
            }
        }

        out[i / 4] = byte;
    }
}

void convertToPalette16(const uint8_t* rgba, u8* out, size_t pixelCount, std::span<const GXRgba> palette) {
    for (size_t i = 0; i < pixelCount; i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2 && (i + j) < pixelCount; ++j) {
            const u8 r = rgba[(i + j) * 4 + 0];
            const u8 g = rgba[(i + j) * 4 + 1];
            const u8 b = rgba[(i + j) * 4 + 2];
            const u8 a = rgba[(i + j) * 4 + 3];

            const auto index = findPaletteIndexGXRgba(GXRgba::fromRGBA(r, g, b, a), palette);
            if (index != -1) {
                byte |= (index & 0x0F) << (j * 4);
            }
        }

        out[i / 2] = byte;
    }
}

void convertToPalette256(const uint8_t* rgba, u8* out, size_t pixelCount, std::span<const GXRgba> palette) {
    for (size_t i = 0; i < pixelCount; i++) {
        const u8 r = rgba[i * 4 + 0];
        const u8 g = rgba[i * 4 + 1];
        const u8 b = rgba[i * 4 + 2];
        const u8 a = rgba[i * 4 + 3];

        const auto index = findPaletteIndexGXRgba(GXRgba::fromRGBA(r, g, b, a), palette);
        if (index != -1) {
            out[i] = index;
        }
    }
}

void convertToDirect(const u8* rgba, GXRgba* out, size_t pixelCount) {
    for (size_t i = 0; i < pixelCount; i++) {
        const u8 r = rgba[i * 4 + 0];
        const u8 g = rgba[i * 4 + 1];
        const u8 b = rgba[i * 4 + 2];
        const u8 a = rgba[i * 4 + 3];

        out[i] = GXRgba::fromRGBA(r, g, b, a);
    }
}

}

bool SPLTexture::convertFromRGBA8888(
    const u8* data,
    s32 width,
    s32 height,
    TextureFormat format,
    std::vector<u8>& outData,
    std::vector<u8>& outPalette) {

    const auto pixelCount = (size_t)width * height;
    switch (format) {
    case TextureFormat::None:
    case TextureFormat::Comp4x4:
    case TextureFormat::Count:
        spdlog::error("Unsupported texture format: {}", (int)format);
        return false;
    case TextureFormat::A3I5:
        outData.resize(pixelCount * sizeof(PixelA3I5));
        outPalette = extractPaletteGXRgb(data, pixelCount, 32);
        convertToA3I5(data, (PixelA3I5*)outData.data(), pixelCount, asSpan<GXRgb>(outPalette));
        break;
    case TextureFormat::Palette4:
        outData.resize(pixelCount / 4);
        outPalette = extractPaletteGXRgba(data, pixelCount, 4);
        convertToPalette4(data, outData.data(), pixelCount, asSpan<GXRgba>(outPalette));
        break;
    case TextureFormat::Palette16:
        outData.resize(pixelCount / 2);
        outPalette = extractPaletteGXRgba(data, pixelCount, 16);
        convertToPalette16(data, outData.data(), pixelCount, asSpan<GXRgba>(outPalette));
        break;
    case TextureFormat::Palette256:
        outData.resize(pixelCount);
        outPalette = extractPaletteGXRgba(data, pixelCount, 256);
        convertToPalette256(data, outData.data(), pixelCount, asSpan<GXRgba>(outPalette));
        break;
    case TextureFormat::A5I3:
        outData.resize(pixelCount * sizeof(PixelA5I3));
        outPalette = extractPaletteGXRgb(data, pixelCount, 32);
        convertToA5I3(data, (PixelA5I3*)outData.data(), pixelCount, asSpan<GXRgb>(outPalette));
        break;
    case TextureFormat::Direct:
        outData.resize(pixelCount * sizeof(GXRgba));
        outPalette.resize(0);
        convertToDirect(data, (GXRgba*)outData.data(), pixelCount);
        break;
    }

    return true;
}
