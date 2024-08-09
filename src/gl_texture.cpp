#include "gl_texture.h"
#include "spl/spl_resource.h"
#include "gl_util.h"

#include <gl/glew.h>


struct PixelA3I5 {
    u8 color : 5;
    u8 alpha : 3;

    u8 getAlpha() const {
        return (alpha << 5) | (alpha << 2) | (alpha >> 1);
    }
};

struct PixelA5I3 {
    u8 color : 3;
    u8 alpha : 5;

    u8 getAlpha() const {
        return (alpha << 3) | (alpha >> 2);
    }
};


GLTexture::GLTexture(const SPLTexture& texture) : m_width(texture.width), m_height(texture.height) {
    createTexture(texture);
}

GLTexture::GLTexture(GLTexture&& other) noexcept {
    if (this != &other) {
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;

        other.m_texture = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_format = TextureFormat::None;
    }
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
    if (this != &other) {
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;

        other.m_texture = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_format = TextureFormat::None;
    }

    return *this;
}

GLTexture::~GLTexture() {
    if (m_texture == 0) { // Was moved from
        return;
    }

    glCall(glDeleteTextures(1, &m_texture));
}

void GLTexture::bind() const {
    glCall(glBindTexture(GL_TEXTURE_2D, m_texture));
}

void GLTexture::unbind() {
    glCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void GLTexture::createTexture(const SPLTexture& texture) {

    // Texture creation is a 2 step process. First the texture/palette data must be converted
    // to a format that OpenGL can understand (RGBA32). Then the texture will be uploaded to the GPU.

    // Step 1: Conversion
    std::vector<u8> textureData;
    switch ((TextureFormat)texture.param.format) {
    case TextureFormat::None:
        break;
    case TextureFormat::A3I5:
        textureData = convertA3I5(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size()
        );
        break;
    case TextureFormat::Palette4:
        textureData = convertPalette4(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size(),
            texture.param.palColor0Transparent
        );
        break;
    case TextureFormat::Palette16:
        textureData = convertPalette16(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size(),
            texture.param.palColor0Transparent
        );
        break;
    case TextureFormat::Palette256:
        textureData = convertPalette256(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size(),
            texture.param.palColor0Transparent
        );
        break;
    case TextureFormat::Comp4x4:
        textureData = convertComp4x4(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size()
        );
        break;
    case TextureFormat::A5I3:
        textureData = convertA5I3(
            texture.textureData.data(),
            (const GXRgba*)texture.paletteData.data(),
            texture.width,
            texture.height,
            texture.paletteData.size()
        );
        break;
    case TextureFormat::Direct:
        textureData = convertDirect(
            (const GXRgba*)texture.textureData.data(),
            texture.width,
            texture.height
        );
        break;
    }

    const auto repeat = (TextureRepeat)texture.param.repeat;

    // Step 2: Upload to GPU
    glCall(glGenTextures(1, &m_texture));
    glCall(glBindTexture(GL_TEXTURE_2D, m_texture));

    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    glCall(glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        repeat == TextureRepeat::S || repeat == TextureRepeat::ST ? GL_REPEAT : GL_CLAMP_TO_EDGE
    ));
    glCall(glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        repeat == TextureRepeat::T || repeat == TextureRepeat::ST ? GL_REPEAT : GL_CLAMP_TO_EDGE
    ));

    // Required for glTextureView (see spl_archive.cpp)
    glCall(glTexStorage2D(
        GL_TEXTURE_2D,
        1,
        GL_RGBA8,
        (s32)m_width,
        (s32)m_height
    ));

    glCall(glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        (s32)m_width,
        (s32)m_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        textureData.data()
    ));

    int immutableFormat;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutableFormat);
    assert(immutableFormat == GL_TRUE);

    glCall(glBindTexture(GL_TEXTURE_2D, 0));
}


std::vector<u8> GLTexture::convertA3I5(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize) {
    std::vector<u8> texture(width * height * 4);
    const auto pixels = reinterpret_cast<const PixelA3I5*>(tex);

    for (size_t i = 0; i < width * height; i++) {
        GXRgba color = pal[pixels[i].color];

        texture[i * 4 + 0] = color.r8();
        texture[i * 4 + 1] = color.g8();
        texture[i * 4 + 2] = color.b8();
        texture[i * 4 + 3] = pixels[i].getAlpha();
    }

    return texture;
}

std::vector<u8> GLTexture::convertPalette4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent) {
    std::vector<u8> texture(width * height * 4);
    const auto pixels = tex;
    const u8 alpha0 = color0Transparent ? 0 : 0xFF;

    for (size_t i = 0; i < width * height; i += 4) {
        const u8 pixel = pixels[i / 4];

        u8 index = pixel & 0x3;
        texture[i * 4 + 0] = pal[index].r8();
        texture[i * 4 + 1] = pal[index].g8();
        texture[i * 4 + 2] = pal[index].b8();
        texture[i * 4 + 3] = index == 0 ? alpha0 : 0xFF;

        index = (pixel >> 2) & 0x3;
        texture[i * 4 + 4] = pal[index].r8();
        texture[i * 4 + 5] = pal[index].g8();
        texture[i * 4 + 6] = pal[index].b8();
        texture[i * 4 + 7] = index == 0 ? alpha0 : 0xFF;

        index = (pixel >> 4) & 0x3;
        texture[i * 4 + 8] = pal[index].r8();
        texture[i * 4 + 9] = pal[index].g8();
        texture[i * 4 + 10] = pal[index].b8();
        texture[i * 4 + 11] = index == 0 ? alpha0 : 0xFF;

        index = (pixel >> 6) & 0x3;
        texture[i * 4 + 12] = pal[index].r8();
        texture[i * 4 + 13] = pal[index].g8();
        texture[i * 4 + 14] = pal[index].b8();
        texture[i * 4 + 15] = index == 0 ? alpha0 : 0xFF;
    }

    return texture;
}

std::vector<u8> GLTexture::convertPalette16(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent) {
    std::vector<u8> texture(width * height * 4);
    const auto pixels = tex;
    const u8 alpha0 = color0Transparent ? 0 : 0xFF;

    for (size_t i = 0; i < width * height; i += 2) {
        const u8 pixel = pixels[i / 2];

        u8 index = pixel & 0xF;
        texture[i * 4 + 0] = pal[index].r8();
        texture[i * 4 + 1] = pal[index].g8();
        texture[i * 4 + 2] = pal[index].b8();
        texture[i * 4 + 3] = index == 0 ? alpha0 : 0xFF;

        index = (pixel >> 4) & 0xF;
        texture[i * 4 + 4] = pal[index].r;
        texture[i * 4 + 5] = pal[index].g;
        texture[i * 4 + 6] = pal[index].b;
        texture[i * 4 + 7] = index == 0 ? alpha0 : 0xFF;
    }

    return texture;
}

std::vector<u8> GLTexture::convertPalette256(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize, bool color0Transparent) {
    std::vector<u8> texture(width * height * 4);
    const auto pixels = tex;
    const u8 alpha0 = color0Transparent ? 0 : 0xFF;

    for (size_t i = 0; i < width * height; i++) {
        const u8 pixel = pixels[i];
        const u8 index = pixel;
        texture[i * 4 + 0] = pal[index].r8();
        texture[i * 4 + 1] = pal[index].g8();
        texture[i * 4 + 2] = pal[index].b8();
        texture[i * 4 + 3] = index == 0 ? alpha0 : 0xFF;
    }

    return texture;
}

std::vector<u8> GLTexture::convertComp4x4(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize) {
    spdlog::warn("GLTexture::convertComp4x4 not implemented");
    return {};
}

std::vector<u8> GLTexture::convertA5I3(const u8* tex, const GXRgba* pal, size_t width, size_t height, size_t palSize) {
    std::vector<u8> texture(width * height * 4);
    const auto pixels = reinterpret_cast<const PixelA5I3*>(tex);

    for (size_t i = 0; i < width * height; i++) {
        GXRgba color = pal[pixels[i].color];

        texture[i * 4 + 0] = color.r8();
        texture[i * 4 + 1] = color.g8();
        texture[i * 4 + 2] = color.b8();
        texture[i * 4 + 3] = pixels[i].getAlpha();
    }

    return texture;
}

std::vector<u8> GLTexture::convertDirect(const GXRgba* tex, size_t width, size_t height) {
    std::vector<u8> texture(width * height * 4);

    for (size_t i = 0; i < width * height; i++) {
        texture[i * 4 + 0] = tex[i].r8();
        texture[i * 4 + 1] = tex[i].g8();
        texture[i * 4 + 2] = tex[i].b8();
        texture[i * 4 + 3] = tex[i].a ? 0xFF : 0x00;
    }

    return texture;
}
