#include "gl_shader.h"

GLShader::GLShader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    std::ifstream vertexFile(vertexPath);
    std::ifstream fragmentFile(fragmentPath);
    if (!vertexFile.is_open()) {
        spdlog::error("Failed to open vertex shader file: {}", vertexPath.string());
        return;
    }
    if (!fragmentFile.is_open()) {
        spdlog::error("Failed to open fragment shader file: {}", fragmentPath.string());
        return;
    }
    std::string vertexSource((std::istreambuf_iterator(vertexFile)), std::istreambuf_iterator<char>());
    std::string fragmentSource((std::istreambuf_iterator(fragmentFile)), std::istreambuf_iterator<char>());
    compileShader(vertexSource, fragmentSource);
}

GLShader::GLShader(std::string_view vertexSource, std::string_view fragmentSource) {
    compileShader(vertexSource, fragmentSource);
}

GLShader::~GLShader() {
    glCall(glDeleteProgram(m_shader));
}

void GLShader::bind() const {
    glCall(glUseProgram(m_shader));
}

void GLShader::unbind() const {
    glCall(glUseProgram(0));
}

u32 GLShader::getUniform(const std::string& name) {
    const auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }

    const auto location = glGetUniformLocation(m_shader, name.c_str());
    m_uniformCache.emplace(name, location);
    return location;
}

void GLShader::compileShader(std::string_view vertexSource, std::string_view fragmentSource) {
    const u32 vs = glCreateShader(GL_VERTEX_SHADER);
    const u32 fs = glCreateShader(GL_FRAGMENT_SHADER);

    const auto vertexSourceData = vertexSource.data();
    const auto fragmentSourceData = fragmentSource.data();

    glCall(glShaderSource(vs, 1, &vertexSourceData, nullptr));
    glCall(glCompileShader(vs));

    glCall(glShaderSource(fs, 1, &fragmentSourceData, nullptr));
    glCall(glCompileShader(fs));

    s32 success;
    char info[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, sizeof(info), nullptr, info);
        spdlog::error("Failed to compile vertex shader: {}", info);
        return;
    }

    glCall(glGetShaderiv(fs, GL_COMPILE_STATUS, &success));
    if (!success) {
        glCall(glGetShaderInfoLog(fs, sizeof(info), nullptr, info));
        spdlog::error("Failed to compile fragment shader: {}", info);
        return;
    }

    m_shader = glCreateProgram();
    if (m_shader == 0) {
        spdlog::error("Failed to create shader program: {}", glGetError());
        return;
    }

    glCall(glAttachShader(m_shader, vs));
    glCall(glAttachShader(m_shader, fs));
    glCall(glLinkProgram(m_shader));

    glCall(glGetProgramiv(m_shader, GL_LINK_STATUS, &success));
    if (!success) {
        glCall(glGetProgramInfoLog(m_shader, sizeof(info), nullptr, info));
        spdlog::error("Failed to link shader program: {}", info);
        return;
    }

    glCall(glDeleteShader(vs));
    glCall(glDeleteShader(fs));
}
