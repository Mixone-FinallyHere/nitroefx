#include "spl_resource.h"

#include <unordered_set>


namespace {

struct TextureStats {
    std::unordered_set<u32> uniqueColors;
    std::unordered_set<u8> uniqueAlphas;
    TextureAttributes flags;
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

TextureStats collectStats(const u8* data, s32 width, s32 height, s32 channels) {
    TextureStats stats = { .flags = TextureAttributes::None };
    const auto data32 = (const u32*)data;
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; ++x) {
            const u32 color = data32[y * width + x];
            const u8 alpha = color >> 24;
            stats.uniqueColors.insert(color & 0xFFFFFF);
            stats.uniqueAlphas.insert(alpha);

            if (alpha != 0xFF) {
                if (alpha == 0) {
                    stats.flags |= TextureAttributes::HasTransparentPixels;
                } else {
                    stats.flags |= TextureAttributes::HasTranslucentPixels;
                }
            }
        }
    }

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
    const auto uniqueColors = (s32)stats.uniqueColors.size();
    const auto uniqueAlphas = (s32)stats.uniqueAlphas.size();
    color0Transparent = stats.flags & TextureAttributes::HasTransparentPixels;
    if (!(stats.flags & TextureAttributes::HasTranslucentPixels)) {
        // No translucency, so we can use one of the palette formats
        return getOpaqueFormatBasedOnUniqueColors(uniqueColors);
    }

    // We have translucent pixels, so we need to use either A3I5 or A5I3.
    // Depending on the number of unique alphas and colors,
    // compressing either color, alpha, or both might be needed.
    if (uniqueAlphas <= 8) {
        // Alpha values can be represented with 3 bits, A3I5 potentially viable
        requiresColorCompression = uniqueColors > 32; // 32 colors is the limit for A3I5
        return TextureFormat::A3I5;
    }
    if (uniqueAlphas <= 32) {
        // Alpha values can be represented with 5 bits, A5I3 potentially viable
        // Check user preference on color vs alpha depth
        if (preference == TextureConversionPreference::AlphaDepth) {
            // User prefers alpha depth over color depth
            requiresColorCompression = uniqueColors > 8; // 8 colors is the limit for A5I3
            return TextureFormat::A5I3;
        }

        // User prefers color depth over alpha depth
        if (uniqueColors <= 8) {
            // Color values can be represented with 3 bits, A5I3 can be used without any compression
            return TextureFormat::A5I3;
        }

        // Too many unique colors, so we should use A3I5
        requiresAlphaCompression = true;
        requiresColorCompression = uniqueColors > 32; // 32 colors is the limit for A3I5
        return TextureFormat::A3I5;
    }

    // Too many unique alpha values, decide between A3I5 and A5I3
    requiresAlphaCompression = true;
    const bool preferColor = preference == TextureConversionPreference::ColorDepth;
    requiresColorCompression = preferColor ? uniqueColors > 32 : uniqueColors > 8;
    return preferColor ? TextureFormat::A3I5 : TextureFormat::A5I3;
}

}

void TextureImportSpecification::setFormat(TextureFormat format) {
    if (this->format == format || format == TextureFormat::None) {
        return;
    }

    const bool transparency = flags & TextureAttributes::HasTransparentPixels;
    const bool translucency = flags & TextureAttributes::HasTranslucentPixels;

    this->format = format;
    switch (format) {
    case TextureFormat::None: return; // Shouldn't happen
    case TextureFormat::A3I5: {
        requiresColorCompression = uniqueColors.size() > 32;
        requiresAlphaCompression = uniqueAlphas.size() > 8;
    } break;
    case TextureFormat::Palette4: {
        requiresColorCompression = (uniqueColors.size() + transparency) > 4;
        requiresAlphaCompression = transparency;
    } break;
    case TextureFormat::Palette16: {
        requiresColorCompression = (uniqueColors.size() + transparency) > 16;
        requiresAlphaCompression = transparency;
    } break;
    case TextureFormat::Palette256: {
        requiresColorCompression = (uniqueColors.size() + transparency) > 256;
        requiresAlphaCompression = transparency;
    } break;
    case TextureFormat::A5I3: {
        requiresColorCompression = uniqueColors.size() > 8;
        requiresAlphaCompression = uniqueAlphas.size() > 32;
    } break;
    case TextureFormat::Direct: {
        requiresColorCompression = uniqueColors.size() > 0x7FFF;
        requiresAlphaCompression = translucency;
    } break;
    }
}

int TextureImportSpecification::getMaxColors() const {
    switch (format) {
    case TextureFormat::None: return 0;
    case TextureFormat::A3I5: return 32;
    case TextureFormat::Palette4: return 4 - color0Transparent;
    case TextureFormat::Palette16: return 16 - color0Transparent;
    case TextureFormat::Palette256: return 256 - color0Transparent;
    case TextureFormat::A5I3: return 8;
    case TextureFormat::Direct: return 0x7FFF;
    }

    return 0;
}

TextureImportSpecification SPLTexture::suggestSpecification(
    s32 width, 
    s32 height, 
    s32 channels, 
    const u8* data,
    TextureConversionPreference preference) {

    const auto stats = collectStats(data, width, height, channels);

    TextureFormat format;
    bool color0Transparent = false;
    bool requiresColorCompression = stats.uniqueColors.size() > 0x7FFF; // Requires color comp anyway if it doesn't fit in a Direct format
    bool requiresAlphaCompression = false;

    switch (channels) {
    case 1: // Grayscale without alpha
        format = getOpaqueFormatBasedOnUniqueColors((s32)stats.uniqueColors.size());
        break;

    case 2: // Grayscale with alpha
        format = getAlphaEnabledFormat(
            stats, 
            preference, 
            color0Transparent, 
            requiresColorCompression, 
            requiresAlphaCompression
        );
        break;

    case 3: // RGB
        format = getOpaqueFormatBasedOnUniqueColors((s32)stats.uniqueColors.size());
        break;
    
    case 4: // RGBA
        format = getAlphaEnabledFormat(
            stats, 
            preference, 
            color0Transparent,
            requiresColorCompression,
            requiresAlphaCompression
        );
        break;

    default: // Unsupported format
        format = TextureFormat::Direct;
        break;
    }

    return {
        .color0Transparent = color0Transparent,
        .requiresColorCompression = requiresColorCompression,
        .requiresAlphaCompression = requiresAlphaCompression,
        .format = format,
        .uniqueColors = stats.uniqueColors,
        .uniqueAlphas = stats.uniqueAlphas,
        .flags = stats.flags
    };
}
