#include "editor_instance.h"
#include "project_manager.h"
#include "random.h"
#include "gfx/gl_util.h"

#include <gl/glew.h>
#include <imgui.h>
#include <random>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


EditorInstance::EditorInstance(const std::filesystem::path& path)
    : m_path(path), m_archive(path), m_particleSystem(1000, m_archive.getTextures())
    , m_camera(glm::radians(45.0f), { 800, 800 }, 1.0f, 500.0f) {
    m_uniqueID = random::nextU64();

    m_updateProj = true;
}

std::pair<bool, bool> EditorInstance::render() {
    bool open = true;
    bool active = false;

    m_camera.setViewportHovered(false);

    const auto name = m_modified ? m_path.filename().string() + "*" : m_path.filename().string();
    if (ImGui::BeginTabItem(name.c_str(), &open)) {
        active = true;
        m_camera.setActive(true);

        const ImVec2 size = ImGui::GetContentRegionAvail();
        m_size = { size.x, size.y };

        ImGui::Image((ImTextureID)(uintptr_t)m_viewport.getTexture(), size);
        if (ImGui::IsItemHovered()) {
            m_camera.setViewportHovered(true);
        }

        ImGui::EndTabItem();
    } else {
        m_camera.setActive(false);
    }

    return { open, active };
}

void EditorInstance::renderParticles(const std::vector<Renderer*>& renderers) {
    if (m_updateProj || m_size != m_viewport.getSize()) {
        m_viewport.resize(m_size);
        m_camera.setViewport(m_size.x, m_size.y);
    }

    m_viewport.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const auto renderer : renderers) {
        renderer->render(m_camera.getView(), m_camera.getProj());
    }

    m_particleSystem.render(m_camera.getParams());

    m_viewport.unbind();
}

void EditorInstance::updateParticles(float deltaTime) {
    m_camera.update();
    m_particleSystem.update(deltaTime);
}

void EditorInstance::handleEvent(const SDL_Event& event) {
    m_camera.handleEvent(event);

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
            m_camera.reset();
        }
    }
}

bool EditorInstance::notifyClosing() {
    return true;
}

bool EditorInstance::valueChanged(bool changed) {
    m_modified |= changed;
    return changed;
}
