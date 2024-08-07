#include "editor_instance.h"
#include "project_manager.h"

#include <imgui.h>
#include <random>

namespace {

std::random_device s_rd;
std::mt19937_64 s_gen(s_rd());

}


EditorInstance::EditorInstance(const std::filesystem::path& path)
    : m_path(path), m_archive(path) {
    m_uniqueID = s_gen();
}

std::pair<bool, bool> EditorInstance::render() {
    bool open = true;
    bool active = false;
    const auto name = m_modified ? m_path.filename().string() + "*" : m_path.filename().string();
    if (ImGui::BeginTabItem(name.c_str(), &open)) {
        active = true;
        ImGui::Text("Editor for %s", m_path.string().c_str());
        ImGui::Text("Textures:");

        for (const auto& texture : m_archive.getTextures()) {
            ImGui::Image((ImTextureID)(uintptr_t)texture.glTexture->getHandle(), { 128, 128 });
        }

        ImGui::EndTabItem();
    }

    return { open, active };
}

bool EditorInstance::notifyClosing() {
    return true;
}

bool EditorInstance::valueChanged(bool changed) {
    m_modified |= changed;
    return changed;
}
