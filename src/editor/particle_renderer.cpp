#include "particle_renderer.h"
#include "gfx/gl_util.h"

#include <algorithm>
#include <gl/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>
#include <ranges>
#include <spdlog/spdlog.h>

namespace {

constexpr f32 s_quadVertices[12] = {
    -1.0f, -1.0f, 0.0f, // bottom left
     1.0f, -1.0f, 0.0f, // bottom right
     1.0f,  1.0f, 0.0f, // top right
    -1.0f,  1.0f, 0.0f  // top left
};

constexpr u32 s_quadIndices[6] = {
    0, 1, 2,
    2, 3, 0
};

constexpr auto s_vertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in mat4 transform;
layout(location = 6) in vec2 texCoords[4];

out vec4 fragColor;
out vec2 texCoord;

uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position = proj * view * transform * vec4(position, 1.0);
    fragColor = color;
    texCoord = texCoords[gl_VertexID];
}
)";

constexpr auto s_fragmentShader = R"(
#version 450 core

layout(location = 0) out vec4 color;

in vec4 fragColor;
in vec2 texCoord;

uniform sampler2D tex;

void main() {
    vec4 outColor = fragColor * texture(tex, texCoord);
    if (outColor.a < 0.1) {
        discard;
    }

    color = outColor;
}
)";

}

ParticleRenderer::ParticleRenderer(u32 maxInstances, std::span<const SPLTexture> textures)
    : m_maxInstances(maxInstances), m_textures(textures), m_view(1.0f), m_proj(1.0f) {

    for (u32 i = 0; i < textures.size(); i++) {
        m_particles.emplace_back();
        m_particles.back().reserve(maxInstances / textures.size()); // Rough distribution for fewer reallocations
    }

    // Create VAO
    glCall(glGenVertexArrays(1, &m_vao));
    glCall(glBindVertexArray(m_vao));

    glCall(glGenBuffers(1, &m_vbo));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices), s_quadVertices, GL_STATIC_DRAW));

    glCall(glEnableVertexAttribArray(0));
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr));

    glCall(glGenBuffers(1, &m_ibo));
    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));
    glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_quadIndices), s_quadIndices, GL_STATIC_DRAW));

    glCall(glGenBuffers(1, &m_transformVbo));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_transformVbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, m_maxInstances * sizeof(ParticleInstance), nullptr, GL_DYNAMIC_DRAW));

    // Color
    glCall(glEnableVertexAttribArray(1));
    glCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), nullptr));
    glCall(glVertexAttribDivisor(1, 1));

    // Transform
    for (u32 i = 0; i < 4; i++) {
        const size_t offset = offsetof(ParticleInstance, transform) + sizeof(glm::vec4) * i;
        glCall(glEnableVertexAttribArray(2 + i));
        glCall(glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offset));
        glCall(glVertexAttribDivisor(2 + i, 1));
    }

    // Tex Coords
    for (u32 i = 0; i < 4; i++) {
        const size_t offset = offsetof(ParticleInstance, texCoords) + sizeof(glm::vec2) * i;
        glCall(glEnableVertexAttribArray(6 + i));
        glCall(glVertexAttribPointer(6 + i, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offset));
        glCall(glVertexAttribDivisor(6 + i, 1));
    }

    glCall(glBindVertexArray(0));

    // Create Shaders
    const u32 vs = glCreateShader(GL_VERTEX_SHADER);
    glCall(glShaderSource(vs, 1, &s_vertexShader, nullptr));
    glCall(glCompileShader(vs));

    const u32 fs = glCreateShader(GL_FRAGMENT_SHADER);
    glCall(glShaderSource(fs, 1, &s_fragmentShader, nullptr));
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

    // Get uniform locations
    glCall(glUseProgram(m_shader));
    m_viewLocation = glGetUniformLocation(m_shader, "view");
    m_projLocation = glGetUniformLocation(m_shader, "proj");
    m_textureLocation = glGetUniformLocation(m_shader, "tex");
    glCall(glUseProgram(0));
}

void ParticleRenderer::begin(const glm::mat4& view, const glm::mat4& proj) {
    for (auto& particles : m_particles) {
        particles.clear();
    }

    m_particleCount = 0;
    m_view = view;
    m_proj = proj;
}

void ParticleRenderer::end() {

    glCall(glUseProgram(m_shader));
    glCall(glActiveTexture(GL_TEXTURE0));
    glCall(glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, glm::value_ptr(m_view)));
    glCall(glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, glm::value_ptr(m_proj)));
    glCall(glUniform1i(m_textureLocation, 0));
    glCall(glBindVertexArray(m_vao));

    for (u32 i = 0; i < m_particles.size(); i++) {
        if (m_particles[i].empty()) {
            continue;
        }

        glCall(glBindTexture(GL_TEXTURE_2D, m_textures[i].glTexture->getHandle()));
        glCall(glBindBuffer(GL_ARRAY_BUFFER, m_transformVbo));
        glCall(glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles[i].size() * sizeof(ParticleInstance), m_particles[i].data()));
        glCall(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (s32)m_particles[i].size()));
    }

    glCall(glBindVertexArray(0));
    glCall(glUseProgram(0));
}

void ParticleRenderer::submit(u32 texture, const ParticleInstance& instance) {
    if (m_particleCount >= m_maxInstances) {
        return;
    }

    if (texture >= m_textures.size()) {
        spdlog::warn("Invalid texture index: {}", texture);
        texture = 0;
    }

    m_particles[texture].push_back(instance);
    ++m_particleCount;
}
