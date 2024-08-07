#include "editor_instance.h"
#include "project_manager.h"

#include <imgui.h>


EditorInstance::EditorInstance(const std::filesystem::path& path)
    : m_path(path), m_archive(path) {
}

bool EditorInstance::render() {
    bool open = true;
    if (ImGui::BeginTabItem(m_path.filename().string().c_str(), &open)) {
        ImGui::Text("Editor for %s", m_path.string().c_str());
        ImGui::Text("Textures:");

        for (const auto& texture : m_archive.getTextures()) {
            ImGui::Image((ImTextureID)(uintptr_t)texture.glTexture->getHandle(), { 128, 128 });
        }

        ImGui::EndTabItem();
    }

    return open;
}

bool EditorInstance::notifyClosing() {
    return true;
}

bool EditorInstance::valueChanged(bool changed) {
    m_modified |= changed;
    return changed;
}
