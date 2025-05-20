#include "editor_instance.h"
#include "application.h"
#include "project_manager.h"
#include "spl/spl_random.h"
#include "gfx/gl_util.h"

#include <GL/glew.h>
#include <imgui.h>
#include <random>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


EditorInstance::EditorInstance(const std::filesystem::path& path, bool isTemp)
    : m_path(path), m_archive(path), m_particleSystem(1000, m_archive.getTextures())
    , m_camera(glm::radians(45.0f), { 800, 800 }, 1.0f, 500.0f), m_isTemp(isTemp) {
    m_uniqueID = SPLRandom::nextU64();
    m_updateProj = true;
    notifyResourceChanged(0);
}

std::pair<bool, bool> EditorInstance::render() {
    bool open = true;
    bool active = false;

    m_camera.setViewportHovered(false);

    if (m_isTemp) {
        ImGui::PushFont(g_application->getFont("Italic"));
    }

    const auto name = m_modified ? m_path.filename().string() + "*" : m_path.filename().string();
    const bool openTab = ImGui::BeginTabItem(name.c_str(), &open);

    if (m_isTemp) {
        ImGui::PopFont();
    }

    // Double click to persist the editor
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        m_isTemp = false;
    }

    if (openTab) {
        active = true;
        m_camera.setActive(true);

        const ImVec2 size = ImGui::GetContentRegionAvail();
        m_size = { size.x, size.y };
        m_size = glm::abs(m_size); // For some reason size.y is sometimes negative idk

        ImGui::Image((ImTextureID)(uintptr_t)m_viewport.getTexture(), size, ImVec2(0, 1), ImVec2(1, 0));
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
        m_updateProj = false;
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

    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_R && event.key.mod & SDL_KMOD_CTRL) {
            m_camera.reset();
        }
    }
}

bool EditorInstance::notifyClosing() {
    return true;
}

void EditorInstance::notifyResourceChanged(size_t index) {
    if (index == -1) {
        m_selectedResource = -1;
        return;
    }

    if (index >= m_archive.getResources().size()) {
        return;
    }

    m_selectedResource = index;
    m_resourceBefore = m_archive.getResources().at(index).duplicate();
}

bool EditorInstance::valueChanged(bool changed) {
    if (m_selectedResource >= m_archive.getResources().size()) {
        return false;
    }

    if (changed) {
        m_isTemp = false; // Changing anything inside a "temporary" editor will make it persistent
    }

    m_modified |= changed;

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        const auto after = m_archive.getResources().at(m_selectedResource).duplicate();

        m_history.push({
            .type = EditorActionType::ResourceModify,
            .resourceIndex = m_selectedResource,
            .before = m_resourceBefore,
            .after = after
        });

        m_resourceBefore = after.duplicate();
    }

    return changed;
}

void EditorInstance::duplicateResource(size_t index) {
    if (index >= m_archive.getResources().size()) {
        return;
    }

    auto& resources = m_archive.getResources();
    const auto& resource = resources[index];
    const auto& newResource = resources.emplace_back(resource.duplicate());

    m_history.push({
        .type = EditorActionType::ResourceAdd,
        .resourceIndex = resources.size() - 1,
        .before = {}, // No before state for new resources
        .after = newResource.duplicate()
    });
}

void EditorInstance::deleteResource(size_t index) {
    if (index >= m_archive.getResources().size()) {
        return;
    }

    auto& resources = m_archive.getResources();
    const auto& resource = resources[index];

    m_history.push({
        .type = EditorActionType::ResourceRemove,
        .resourceIndex = index,
        .before = resource.duplicate(),
        .after = {} // No after state for deleted resources
    });

    resources.erase(resources.begin() + index);
}

void EditorInstance::addResource() {
    auto& resources = m_archive.getResources();
    const auto& resource = resources.emplace_back(SPLResource::create());

    m_history.push({
        .type = EditorActionType::ResourceAdd,
        .resourceIndex = resources.size() - 1,
        .before = {}, // No before state for new resources
        .after = resource.duplicate()
    });
}

void EditorInstance::save() {
    if (m_path.empty()) {
        return;
    }

    m_archive.save(m_path);
    m_modified = false;
}

void EditorInstance::saveAs(const std::filesystem::path& path) {
    m_path = path;
    return save();
}

EditorActionType EditorInstance::undo() {
    if (m_history.canUndo()) {
        m_modified = true;
        return m_history.undo(m_archive.getResources());
    }

    return EditorActionType::None;
}

EditorActionType EditorInstance::redo() {
    if (m_history.canRedo()) {
        m_modified = true;
        return m_history.redo(m_archive.getResources());
    }

    return EditorActionType::None;
}
