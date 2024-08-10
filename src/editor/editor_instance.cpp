#include "editor_instance.h"
#include "project_manager.h"
#include "random.h"
#include "gl_util.h"

#include <gl/glew.h>
#include <imgui.h>
#include <random>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


EditorInstance::EditorInstance(const std::filesystem::path& path)
    : m_path(path), m_archive(path), m_particleSystem(1000, m_archive.getTextures()) {
    m_uniqueID = random::nextU64();

    // will be updated in renderParticles
    m_proj = glm::mat4(1.0f);
    m_updateProj = true;
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
    if (m_updateProj || m_size != m_viewport.getSize()) {
        m_viewport.resize(m_size);
        m_proj = glm::perspective(glm::radians(45.0f), m_size.x / m_size.y, 1.0f, 500.0f);
    }

    m_viewport.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_particleSystem.render(
        glm::lookAt(glm::vec3(0, 1, 6), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)),
        m_proj,
        glm::vec3(0, 1, 6)
    );

    m_viewport.unbind();
}

void EditorInstance::updateParticles(float deltaTime) {
    m_particleSystem.update(deltaTime);
}

bool EditorInstance::notifyClosing() {
    return true;
}

bool EditorInstance::valueChanged(bool changed) {
    m_modified |= changed;
    return changed;
}
