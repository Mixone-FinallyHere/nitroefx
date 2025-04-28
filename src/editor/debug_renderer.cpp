#include "debug_renderer.h"

#include <array>

#include <glm/gtc/type_ptr.hpp>


namespace {

using namespace std::string_view_literals;

constexpr auto s_lineVertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 view;
uniform mat4 proj;

out vec3 fragColor;

void main() {
    gl_Position = proj * view * vec4(position, 1.0);
    fragColor = color;
}
)"sv;

constexpr auto s_objectVertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in mat4 model;

uniform mat4 view;
uniform mat4 proj;

out vec3 fragColor;

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    fragColor = color;
}
)"sv;

constexpr auto s_fragmentShader = R"(
#version 450 core

in vec3 fragColor;

out vec4 color;

void main() {
    color = vec4(fragColor, 1.0);
}
)"sv;

static std::array s_boxVertices = {
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
};

static std::array s_boxIndices = {
    0, 1, 2, 2, 3, 0,
    1, 5, 6, 6, 2, 1,
    5, 4, 7, 7, 6, 5,
    4, 0, 3, 3, 7, 4,
    3, 2, 6, 6, 7, 3,
    4, 5, 1, 1, 0, 4
};

}

DebugRenderer::DebugRenderer(u32 maxLines) 
    : m_lineShader(s_lineVertexShader, s_fragmentShader)
    , m_objectShader(s_objectVertexShader, s_fragmentShader) {
    // Lines
    m_vertices.reserve(maxLines * 2);
    m_lineShader.bind();
    m_viewLocation = m_lineShader.getUniform("view");
    m_projLocation = m_lineShader.getUniform("proj");
    m_lineShader.unbind();

    glCall(glGenVertexArrays(1, &m_lineVao));
    glCall(glGenBuffers(1, &m_lineVbo));

    glCall(glBindVertexArray(m_lineVao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, maxLines * sizeof(Vertex) * 2, nullptr, GL_DYNAMIC_DRAW));

    glCall(glEnableVertexAttribArray(0));
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos)));

    glCall(glEnableVertexAttribArray(1));
    glCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color)));

    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    glCall(glBindVertexArray(0));

    // Objects
    m_objectShader.bind();
    m_viewLocation = m_objectShader.getUniform("view");
    m_projLocation = m_objectShader.getUniform("proj");
    m_objectShader.unbind();

    glCall(glGenVertexArrays(1, &m_boxVao));
    glCall(glGenBuffers(1, &m_boxVbo));
    glCall(glGenBuffers(1, &m_boxIbo));

    glCall(glBindVertexArray(m_boxVao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_boxVbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, s_boxVertices.size() * sizeof(f32), s_boxVertices.data(), GL_STATIC_DRAW));

    glCall(glEnableVertexAttribArray(0));
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (const void*)0));

    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxIbo));
    glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_boxIndices.size() * sizeof(u32), s_boxIndices.data(), GL_STATIC_DRAW));

    glCall(glBindVertexArray(0));

    glCall(glGenBuffers(1, &m_boxInstanceVbo));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_boxInstanceVbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW));
    
    glCall(glBindVertexArray(m_boxVao));
    glCall(glEnableVertexAttribArray(1));
    glCall(glEnableVertexAttribArray(2));
    glCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (const void*)offsetof(BoxInstance, color)));
    glCall(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BoxInstance), (const void*)offsetof(BoxInstance, transform)));
    glCall(glVertexAttribDivisor(1, 1));
    glCall(glVertexAttribDivisor(2, 1));
    glCall(glBindVertexArray(0));

    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void DebugRenderer::render(const glm::mat4& view, const glm::mat4& proj) {
    m_lineShader.bind();
    glCall(glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, glm::value_ptr(view)));
    glCall(glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, glm::value_ptr(proj)));

    glCall(glBindVertexArray(m_lineVao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo));
    glCall(glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data()));
    glCall(glDrawArrays(GL_LINES, 0, (u32)m_vertices.size() * 2));
    glCall(glBindVertexArray(0));

    m_lineShader.unbind();

    m_objectShader.bind();
    glCall(glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, glm::value_ptr(view)));
    glCall(glUniformMatrix4fv(m_projLocation, 1, GL_FALSE, glm::value_ptr(proj)));

    glCall(glBindVertexArray(m_boxVao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_boxInstanceVbo));
    glCall(glBufferSubData(GL_ARRAY_BUFFER, 0, m_boxes.size() * sizeof(BoxInstance), m_boxes.data()));
    glCall(glDrawElementsInstanced(GL_TRIANGLES, s_boxIndices.size(), GL_UNSIGNED_INT, nullptr, m_boxes.size()));
    glCall(glBindVertexArray(0));

    m_objectShader.unbind();
    
    m_vertices.clear();
    m_boxes.clear();
}

void DebugRenderer::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    m_vertices.push_back({ start, color });
    m_vertices.push_back({ end, color });
}

void DebugRenderer::addPlane(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& color) {
    addLine(p, p + a, color);
    addLine(p, p + b, color);
    addLine(p + a, p + a + b, color);
    addLine(p + b, p + a + b, color);
}

void DebugRenderer::addBox(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color) {
    BoxInstance instance = {
        .transform = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), scale),
        .color = color
    };

    m_boxes.push_back(instance);
}
