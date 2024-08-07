#pragma once

#include <SDL2/SDL_opengl.h>
#include <vector>

#include "types.h"

struct SPLTexture;

class GLTexture {
public:
    explicit GLTexture(const SPLTexture& texture);
    GLTexture(const GLTexture& other) = delete;
    GLTexture(GLTexture&& other) noexcept;

    GLTexture& operator=(const GLTexture& other) = delete;
    GLTexture& operator=(GLTexture&& other) noexcept;

    ~GLTexture();

    void bind() const;
    static void unbind();

    GLuint getHandle() const { return m_texture; }
    size_t getWidth() const { return m_width; }
    size_t getHeight() const { return m_height; }
    TextureFormat getFormat() const { return m_format; }

private:
    void createTexture(const SPLTexture& texture);

    static std::vector<u8> convertA3I5(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    static std::vector<u8> convertPalette4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    static std::vector<u8> convertPalette16(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    static std::vector<u8> convertPalette256(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent);
    static std::vector<u8> convertComp4x4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    static std::vector<u8> convertA5I3(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize);
    static std::vector<u8> convertDirect(const GXRgba* tex, size_t width, size_t height);

private:
    GLuint m_texture;
    size_t m_width;
    size_t m_height;
    TextureFormat m_format;
};

