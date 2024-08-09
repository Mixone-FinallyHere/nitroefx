#include "particle_renderer.h"

#include <gl/glew.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/type_ptr.hpp>


namespace {

constexpr f32 s_quadVertices[12] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.5f,  0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f
};

constexpr u32 s_quadIndices[6] = {
    0, 1, 2,
    2, 3, 0
};

constexpr auto s_vertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 transform;


uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position = proj * view * transform * vec4(position, 1.0);
}
)";

constexpr auto s_fragmentShader = R"(
#version 450 core

layout(location = 0) out vec4 color;

void main() {
    color = vec4(0.0, 0.5, 1.0, 1.0);
}
)";

#define glCall(x) x; if (const auto error = glGetError()) { \
    spdlog::error("OpenGL error at {}:{} -> {}", __FILE__, __LINE__, error);\
}

}

ParticleRenderer::ParticleRenderer(u32 maxInstances) : m_maxInstances(maxInstances) {
    // Create VAO
    glCall(glGenVertexArrays(1, &m_vao))
    glCall(glBindVertexArray(m_vao))

    glCall(glGenBuffers(1, &m_vbo))
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_vbo))
    glCall(glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices), s_quadVertices, GL_STATIC_DRAW))

    glCall(glEnableVertexAttribArray(0))
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr))

    glCall(glGenBuffers(1, &m_ibo))
    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo))
    glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_quadIndices), s_quadIndices, GL_STATIC_DRAW))

    m_transforms.reserve(m_maxInstances);

    glCall(glGenBuffers(1, &m_transformVbo))
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_transformVbo))
    glCall(glBufferData(GL_ARRAY_BUFFER, m_maxInstances * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW))

    for (u32 i = 0; i < 4; i++) {
        glCall(glEnableVertexAttribArray(1 + i))
        glCall(glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(f32) * i * 4)))
        glCall(glVertexAttribDivisor(1 + i, 1))
    }

    glCall(glBindVertexArray(0))

    // Create Shaders
    const u32 vs = glCreateShader(GL_VERTEX_SHADER);
    glCall(glShaderSource(vs, 1, &s_vertexShader, nullptr))
    glCall(glCompileShader(vs))

    const u32 fs = glCreateShader(GL_FRAGMENT_SHADER);
    glCall(glShaderSource(fs, 1, &s_fragmentShader, nullptr))
    glCall(glCompileShader(fs))

    s32 success;
    char info[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, sizeof(info), nullptr, info);
        spdlog::error("Failed to compile vertex shader: {}", info);
        return;
    }

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, sizeof(info), nullptr, info);
        spdlog::error("Failed to compile fragment shader: {}", info);
        return;
    }

    m_shader = glCreateProgram();
    glCall(glAttachShader(m_shader, vs))
    glCall(glAttachShader(m_shader, fs))
    glCall(glLinkProgram(m_shader))

    glGetShaderiv(m_shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(m_shader, sizeof(info), nullptr, info);
        spdlog::error("Failed to link shader program: {}", info);
        return;
    }

    glCall(glDeleteShader(vs))
    glCall(glDeleteShader(fs))

    glCall(glUseProgram(m_shader))
    m_viewLocation = glGetUniformLocation(m_shader, "view");
    m_projLocation = glGetUniformLocation(m_shader, "proj");
    glCall(glUseProgram(0))
}

void ParticleRenderer::begin(const glm::mat4& view, const glm::mat4& proj) {
    m_transforms.clear();
    m_view = view;
    m_proj = proj;
}

void ParticleRenderer::end() {
    if (m_transforms.empty()) {
        return;
    }

    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_transformVbo));
    glCall(glBufferSubData(GL_ARRAY_BUFFER, 0, m_transforms.size() * sizeof(glm::mat4), m_transforms.data()));

    glCall(glUseProgram(m_shader));
    glCall(glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, glm::value_ptr(m_view)));
    glCall(glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, glm::value_ptr(m_proj)));

    glCall(glBindVertexArray(m_vao));
    glCall(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (s32)m_transforms.size()));
    glCall(glBindVertexArray(0));
    glCall(glUseProgram(0));
}

void ParticleRenderer::submit(const ParticleInstance& instance) {
    m_transforms.push_back(instance.transform);
}
