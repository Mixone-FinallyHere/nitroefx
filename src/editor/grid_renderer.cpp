#include "grid_renderer.h"

#include <glm/gtc/type_ptr.hpp>

namespace {

using namespace std::string_view_literals;

constexpr auto s_lineVertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 proj;
uniform float height;
uniform vec4 color;

out vec4 fragColor;

void main() {
    gl_Position = proj * view * vec4(position.x, height, position.z, 1.0);
    fragColor = color;
}
)"sv;

constexpr auto s_fragmentShader = R"(
#version 450 core

in vec4 fragColor;

out vec4 color;

void main() {
    color = fragColor;
    //color = vec4(1.0, 1.0, 1.0, 0.2);
}
)"sv;

}

GridRenderer::GridRenderer(const glm::ivec2& dimensions, const glm::vec2& spacing)
    : m_dimensions(dimensions), m_spacing(spacing), m_shader(s_lineVertexShader, s_fragmentShader) {
    createGrid();
}

void GridRenderer::render(const glm::mat4& view, const glm::mat4& proj) {
    m_shader.bind();
    glCall(glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, glm::value_ptr(view)));
    glCall(glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, glm::value_ptr(proj)));
    glCall(glUniform1f(m_heightLocation, m_height));
    glCall(glUniform4fv(m_colorLocation, 1, glm::value_ptr(m_color)));
    glCall(glBindVertexArray(m_vao));
    glCall(glDrawArrays(GL_LINES, 0, (u32)m_vertices.size()));
    glCall(glBindVertexArray(0));
    m_shader.unbind();
}

void GridRenderer::createGrid() {
    const auto gridSize = glm::vec2(m_dimensions) * m_spacing;
    const auto halfSize = gridSize * 0.5f;

    for (float x = -halfSize.x; x <= halfSize.x; x += m_spacing.x) {
        m_vertices.emplace_back(x, 0.0f, -halfSize.y);
        m_vertices.emplace_back(x, 0.0f, halfSize.y);
    }

    for (float y = -halfSize.y; y <= halfSize.y; y += m_spacing.y) {
        m_vertices.emplace_back(-halfSize.x, 0.0f, y);
        m_vertices.emplace_back(halfSize.x, 0.0f, y);
    }

    glCall(glGenVertexArrays(1, &m_vao));
    glCall(glBindVertexArray(m_vao));

    glCall(glGenBuffers(1, &m_vbo));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3), m_vertices.data(), GL_STATIC_DRAW));

    glCall(glEnableVertexAttribArray(0));
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr));

    glCall(glBindVertexArray(0));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

    m_shader.bind();
    m_viewLocation = m_shader.getUniform("view");
    m_projLocation = m_shader.getUniform("proj");
    m_heightLocation = m_shader.getUniform("height");
    m_colorLocation = m_shader.getUniform("color");
    m_shader.unbind();
}
