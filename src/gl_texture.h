#pragma once

#include "spl/spl_resource.h"

#include <SDL2/SDL_opengl.h>

enum class TextureFormat {
    A3I5 = 1,
    Palette4,
    Palette16,
    Palette256,
    Comp4x4,
    A5I3,
    Direct,
};

enum class TextureRepeat {
    None,
    S,
    T,
    ST,
};

class GLTexture {
public:
    explicit GLTexture(const SPLTexture& texture);

    ~GLTexture();

    void bind() const;
    void unbind() const;

    size_t getWidth() const { return m_width; }
    size_t getHeight() const { return m_height; }

    GLuint getHandle() const { return m_texture; }

private:
    void createTexture(const SPLTexture& texture);

    std::vector<u8> convertA3I5(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    std::vector<u8> convertPalette4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    std::vector<u8> convertPalette16(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    std::vector<u8> convertPalette256(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    std::vector<u8> convertComp4x4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    std::vector<u8> convertA5I3(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    std::vector<u8> convertDirect(const GXRgba* tex, size_t width, size_t height);

private:
    GLuint m_texture;
    size_t m_width;
    size_t m_height;
    TextureFormat m_format;
};

