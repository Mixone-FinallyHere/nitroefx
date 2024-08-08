#include "editor_instance.h"
#include "project_manager.h"
#include "random.h"

#include <gl/glew.h>
#include <imgui.h>
#include <random>


namespace {

constexpr const char* s_vertexShader = R"(
#version 450 core

layout (location = 0) in vec3 pos;

void main() {
    gl_Position = vec4(pos, 1.0);
}
)";

constexpr const char* s_fragmentShader = R"(
#version 450 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

#define glCall(x) x; if (const auto error = glGetError()) { spdlog::error("OpenGL error: {}", error); }

}


EditorInstance::EditorInstance(const std::filesystem::path& path)
    : m_path(path), m_archive(path) {
    m_uniqueID = random::nextU64();

    static float triangle[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glCall(glGenVertexArrays(1, &m_vertexArray));
    glCall(glBindVertexArray(m_vertexArray));

    glCall(glGenBuffers(1, &m_vertexBuffer));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer));
    glCall(glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW));

    glCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));
    glCall(glEnableVertexAttribArray(0));

    const u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glCall(glShaderSource(vertexShader, 1, &s_vertexShader, nullptr));
    glCall(glCompileShader(vertexShader));

    const u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glCall(glShaderSource(fragmentShader, 1, &s_fragmentShader, nullptr));
    glCall(glCompileShader(fragmentShader));

    m_shaderProgram = glCreateProgram();
    glCall(glAttachShader(m_shaderProgram, vertexShader));
    glCall(glAttachShader(m_shaderProgram, fragmentShader));
    glCall(glLinkProgram(m_shaderProgram));

    glCall(glDeleteShader(vertexShader));
    glCall(glDeleteShader(fragmentShader));
}

std::pair<bool, bool> EditorInstance::render() {
    bool open = true;
    bool active = false;
    const auto name = m_modified ? m_path.filename().string() + "*" : m_path.filename().string();
    if (ImGui::BeginTabItem(name.c_str(), &open)) {
        active = true;

        const ImVec2 size = ImGui::GetContentRegionAvail();
        m_size = { size.x, size.y };

        ImGui::Image((ImTextureID)(uintptr_t)m_viewport.getTexture(), size);

        ImGui::EndTabItem();
    }

    return { open, active };
}

void EditorInstance::renderParticles() {
    if (m_size != m_viewport.getSize()) {
        m_viewport.resize(m_size);
    }

    m_viewport.bind();

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_viewport.unbind();
}

bool EditorInstance::notifyClosing() {
    return true;
}

bool EditorInstance::valueChanged(bool changed) {
    m_modified |= changed;
    return changed;
}
