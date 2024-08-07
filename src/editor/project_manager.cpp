#include "project_manager.h"

#include <imgui.h>

#include "SDL_messagebox.h"
#include "spdlog/spdlog.h"


void ProjectManager::openProject(const std::filesystem::path& path) {
    m_projectPath = path;
}

void ProjectManager::closeProject(bool force) {
    bool canClose = true;
    for (const auto& editor : m_openEditors) {
        canClose &= editor->notifyClosing();
    }

    if (canClose || force) {
        m_activeEditor.reset();
        m_projectPath.clear();
        m_openEditors.clear();
    }
}

void ProjectManager::openEditor(const std::filesystem::path& path) {
    const auto editor = std::make_shared<EditorInstance>(path);
    m_activeEditor = editor;
    m_openEditors.push_back(editor);
}

void ProjectManager::closeEditor(const std::shared_ptr<EditorInstance>& editor) {
    if (editor->notifyClosing()) {
        std::erase(m_openEditors, editor);
        if (m_activeEditor == editor) {
            m_activeEditor.reset();
        }
    }
}

void ProjectManager::open() {
    m_open = true;
}

void ProjectManager::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin("Project Manager##ProjectManager", &m_open)) {
        if (m_projectPath.empty()) {
            ImGui::Text("No project open");
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(m_projectPath)) {
                if (entry.is_directory()) {
                    renderDirectory(entry.path());
                } else {
                    renderFile(entry.path());
                }
            }
        }
    }

    ImGui::End();
}

void ProjectManager::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_DROPFILE) {
        return;
    }

    const std::filesystem::path path = event.drop.file;
    if (std::filesystem::is_directory(path)) {
        openProject(path);
    } else {
        openEditor(path);
    }
}

void ProjectManager::renderDirectory(const std::filesystem::path& path) {
    const bool open = ImGui::TreeNodeEx(path.filename().string().c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth);

    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("New file")) {
            ImGui::CloseCurrentPopup();
            ImGui::OpenPopup("New file##ProjectManager");
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("New file##ProjectManager")) {
        ImGui::Text("New file");
        ImGui::EndPopup();
    }

    if (open) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                renderDirectory(entry.path());
            } else {
                renderFile(entry.path());
            }
        }

        ImGui::TreePop();
    }
}

void ProjectManager::renderFile(const std::filesystem::path& path) {
    if (ImGui::Selectable(path.filename().string().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            openEditor(path);
        }
    }

    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Open")) {
            openEditor(path);
        }

        if (ImGui::MenuItem("Delete")) {
            spdlog::info("Deleting file: {}", path.string());
            // TODO: Actually delete the file
        }

        ImGui::EndPopup();
    }
}
