#include "spl_resource.h"

#include <unordered_set>


namespace {

enum class TextureAttributes {
    None = 0,
    HasTransparentPixels = 1 << 0,
    HasTranslucentPixels = 1 << 1,
};

struct TextureStats {
    s32 uniqueColors;
    TextureAttributes flags;
    s32 uniqueAlphas;
};

TextureAttributes operator|(TextureAttributes a, TextureAttributes b) {
    return static_cast<TextureAttributes>(static_cast<u32>(a) | static_cast<u32>(b));
}
TextureAttributes operator|=(TextureAttributes& a, TextureAttributes b) {
    return a = a | b;
}
bool operator&(TextureAttributes a, TextureAttributes b) {
    return static_cast<u32>(a) & static_cast<u32>(b);
}

TextureStats collectStatsGrayscale(const u8* data, s32 width, s32 height) {
    std::unordered_set<u8> colors;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; ++x) {
            colors.insert(data[y * width + x]);
        }
    }

    return { (s32)colors.size(), TextureAttributes::None, 0 };
}
TextureStats collectStatsGrayscaleAlpha(const u8* data, s32 width, s32 height) {
    TextureStats stats = { 0, TextureAttributes::None, 0 };
    std::unordered_set<u8> colors;
    std::unordered_set<u8> alphas;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; x += 2) {
            colors.insert(data[y * width + x]);
            const u8 alpha = data[y * width + x + 1];
            alphas.insert(alpha);

            if (alpha != 0xFF) {
                if (alpha == 0) {
                    stats.flags |= TextureAttributes::HasTransparentPixels;
                } else {
                    stats.flags |= TextureAttributes::HasTranslucentPixels;
                }
            }
        }
    }

    stats.uniqueColors = (s32)colors.size();
    stats.uniqueAlphas = (s32)alphas.size();

    return stats;
}
TextureStats collectStatsRGB(const u8* data, s32 width, s32 height) {
    std::unordered_set<u32> colors;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; x += 3) {
            colors.insert(*(u32*)(data + y * width + x) & 0xFFFFFF);
        }
    }

    return { (s32)colors.size(), TextureAttributes::None, 0 };
}
TextureStats collectStatsRGBA(const u8* data, s32 width, s32 height) {
    TextureStats stats = { 0, TextureAttributes::None, 0 };
    std::unordered_set<u32> colors;
    std::unordered_set<u8> alphas;
    const auto data32 = (const u32*)data;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; x += 4) {
            const u32 color = data32[y * width + x];
            const u8 alpha = color >> 24;
            colors.insert(color & 0xFFFFFF);
            alphas.insert(alpha);

            if (alpha != 0xFF) {
                if (alpha == 0) {
                    stats.flags |= TextureAttributes::HasTransparentPixels;
                } else {
                    stats.flags |= TextureAttributes::HasTranslucentPixels;
                }
            }
        }
    }

    stats.uniqueColors = (s32)colors.size();
    stats.uniqueAlphas = (s32)alphas.size();

    return stats;
}

TextureStats collectStats(const u8* data, s32 width, s32 height, s32 channels) {
    TextureStats stats = { 0, TextureAttributes::None, 0 };
    std::unordered_set<u32> colors;
    std::unordered_set<u8> alphas;
    const auto data32 = (const u32*)data;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; ++x) {
            const u32 color = data32[y * width + x];
            const u8 alpha = color >> 24;
            colors.insert(color & 0xFFFFFF);
            alphas.insert(alpha);

            if (alpha != 0xFF) {
                if (alpha == 0) {
                    stats.flags |= TextureAttributes::HasTransparentPixels;
                } else {
                    stats.flags |= TextureAttributes::HasTranslucentPixels;
                }
            }
        }
    }

    stats.uniqueColors = (s32)colors.size();
    stats.uniqueAlphas = (s32)alphas.size();

    return stats;
}

TextureFormat getOpaqueFormatBasedOnUniqueColors(s32 uniqueColors) {
    if (uniqueColors <= 4) {
        return TextureFormat::Palette4;
    } else if (uniqueColors <= 16) {
        return TextureFormat::Palette16;
    } else if (uniqueColors <= 256) {
        return TextureFormat::Palette256;
    } else {
        return TextureFormat::Direct;
    }
}

TextureFormat getAlphaEnabledFormat(const TextureStats& stats, TextureConversionPreference preference,
    bool& color0Transparent, bool& requiresColorCompression, bool& requiresAlphaCompression) {
    color0Transparent = stats.flags & TextureAttributes::HasTransparentPixels;
    if (!(stats.flags & TextureAttributes::HasTranslucentPixels)) {
        // No translucency, so we can use one of the palette formats
        return getOpaqueFormatBasedOnUniqueColors(stats.uniqueColors);
    }

    // We have translucent pixels, so we need to use either A3I5 or A5I3.
    // Depending on the number of unique alphas and colors,
    // compressing either color, alpha, or both might be needed.
    if (stats.uniqueAlphas <= 8) {
        // Alpha values can be represented with 3 bits, A3I5 potentially viable
        requiresColorCompression = stats.uniqueColors > 32; // 32 colors is the limit for A3I5
        return TextureFormat::A3I5;
    } else if (stats.uniqueAlphas <= 32) {
        // Alpha values can be represented with 5 bits, A5I3 potentially viable
        // Check user preference on color vs alpha depth
        if (preference == TextureConversionPreference::AlphaDepth) {
            // User prefers alpha depth over color depth
            requiresColorCompression = stats.uniqueColors > 8; // 8 colors is the limit for A5I3
            return TextureFormat::A5I3;
        }

        // User prefers color depth over alpha depth
        if (stats.uniqueColors <= 8) {
            // Color values can be represented with 3 bits, A5I3 can be used without any compression
            return TextureFormat::A5I3;
        } else {
            // Too many unique colors, so we should use A3I5
            requiresAlphaCompression = true;
            requiresColorCompression = stats.uniqueColors > 32; // 32 colors is the limit for A3I5
            return TextureFormat::A3I5;
        }
    } else {
        // Too many unique alpha values, decide between A3I5 and A5I3
        requiresAlphaCompression = true;
        bool preferColor = preference == TextureConversionPreference::ColorDepth;
        requiresColorCompression = preferColor ? stats.uniqueColors > 32 : stats.uniqueColors > 8;
        return preferColor ? TextureFormat::A3I5 : TextureFormat::A5I3;
    }
}

}

TextureFormat SPLTexture::suggestFormat(
    s32 width, 
    s32 height, 
    s32 channels, 
    const u8* data,
    TextureConversionPreference preference,
    bool& color0Transparent, 
    bool& requiresColorCompression,
    bool& requiresAlphaCompression) {

    color0Transparent = false;
    requiresColorCompression = false;
    requiresAlphaCompression = false;

    const auto stats = collectStats(data, width, height, channels);
    switch (channels) {
    case 1: // Grayscale without alpha
        return getOpaqueFormatBasedOnUniqueColors(stats.uniqueColors);

    case 2: // Grayscale with alpha
        return getAlphaEnabledFormat(stats, preference, color0Transparent, requiresColorCompression, requiresAlphaCompression);

    case 3: // RGB
        return getOpaqueFormatBasedOnUniqueColors(stats.uniqueColors);
    
    case 4: // RGBA
        return getAlphaEnabledFormat(stats, preference, color0Transparent, requiresColorCompression, requiresAlphaCompression);

    default:
        return TextureFormat::Direct;
    }
}
