#include "debug_renderer.h"
#include "mesh_generator.h"

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

out vec4 fragColor;

void main() {
    gl_Position = proj * view * vec4(position, 1.0);
    fragColor = vec4(color, 1.0);
}
)"sv;

constexpr auto s_objectVertexShader = R"(
#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in mat4 model;

uniform mat4 view;
uniform mat4 proj;

out vec4 fragColor;

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    fragColor = color;
}
)"sv;

constexpr auto s_fragmentShader = R"(
#version 450 core

in vec4 fragColor;

out vec4 color;

void main() {
    color = fragColor;
}
)"sv;

}

DebugRenderer::DebugRenderer(u32 maxLines, u32 maxBoxes, u32 maxSpheres)
    : m_lineShader(s_lineVertexShader, s_fragmentShader)
    , m_objectShader(s_objectVertexShader, s_fragmentShader)
    , m_maxLines(maxLines) {
    // Lines
    m_vertices.reserve(m_maxLines * 2ull);
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

    const GeneratedMesh boxMesh = MeshGenerator::generateBox({ 1.0f, 1.0f, 1.0f });
    m_boxRenderData.init(boxMesh, maxBoxes);

    const GeneratedMesh sphereMesh = MeshGenerator::generateSphere(1.0f, 16, 16);
    m_sphereRenderData.init(sphereMesh, maxSpheres);
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

    glCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    
    m_boxRenderData.render();
    m_sphereRenderData.render();

    glCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

    m_objectShader.unbind();
    
    m_vertices.clear();
    m_boxRenderData.instances.clear();
    m_sphereRenderData.instances.clear();
}

void DebugRenderer::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color) {
    if (m_vertices.size() >= m_maxLines * 2ull) {
        return;
    }

    m_vertices.push_back({ start, color });
    m_vertices.push_back({ end, color });
}

void DebugRenderer::addPlane(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec4& color) {
    if (m_vertices.size() >= m_maxLines * 2ull) {
        return;
    }

    addLine(p, p + a, color);
    addLine(p, p + b, color);
    addLine(p + a, p + a + b, color);
    addLine(p + b, p + a + b, color);
}

void DebugRenderer::addBox(const glm::vec3& pos, const glm::vec3& scale, const glm::vec4& color) {
    if (m_boxRenderData.instances.size() >= m_boxRenderData.maxInstances) {
        return;
    }

    ObjectInstace instance = {
        .color = color,
        .transform = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), scale),
    };

    m_boxRenderData.instances.push_back(instance);
}

void DebugRenderer::addSphere(const glm::vec3& center, f32 radius, const glm::vec4& color) {
    if (m_sphereRenderData.instances.size() >= m_sphereRenderData.maxInstances) {
        return;
    }

    ObjectInstace instance = {
        .color = color,
        .transform = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), { radius, radius, radius }),
    };

    m_sphereRenderData.instances.push_back(instance);
}

void DebugRenderer::ObjectRenderData::init(const GeneratedMesh& mesh, u32 maxInstances) {
    this->maxInstances = maxInstances;
    indexCount = (u32)mesh.indices.size();
    instances.reserve(maxInstances);

    glCall(glGenVertexArrays(1, &vao));
    glCall(glGenBuffers(1, &vbo));
    glCall(glGenBuffers(1, &ibo));
    glCall(glBindVertexArray(vao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.data(), GL_STATIC_DRAW));

    glCall(glEnableVertexAttribArray(0));
    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr));

    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    glCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(s32), mesh.indices.data(), GL_STATIC_DRAW));

    glCall(glBindVertexArray(0));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    glCall(glGenBuffers(1, &instanceVbo));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVbo));
    glCall(glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(ObjectInstace), nullptr, GL_DYNAMIC_DRAW));

    glCall(glBindVertexArray(vao));
    glCall(glEnableVertexAttribArray(1));
    glCall(glEnableVertexAttribArray(2));

    glCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectInstace), (const void*)offsetof(ObjectInstace, color)));
    glCall(glVertexAttribDivisor(1, 1));

    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(
            2 + i,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(ObjectInstace),
            (void*)(offsetof(ObjectInstace, transform) + sizeof(glm::vec4) * i)
        );
        glVertexAttribDivisor(2 + i, 1);
    }

    glCall(glBindVertexArray(0));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    glCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void DebugRenderer::ObjectRenderData::render() {
    glCall(glBindVertexArray(vao));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVbo));
    glCall(glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(ObjectInstace), instances.data()));
    glCall(glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, (s32)instances.size()));
    glCall(glBindVertexArray(0));
}
