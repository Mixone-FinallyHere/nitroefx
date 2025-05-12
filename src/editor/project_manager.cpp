#include "project_manager.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <SDL_messagebox.h>
#include <spdlog/spdlog.h>

#include "fonts/IconsFontAwesome6.h"


void ProjectManager::openProject(const std::filesystem::path& path) {
    if (hasProject()) {
        constexpr SDL_MessageBoxButtonData buttons[] = {
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" }
        };
        const SDL_MessageBoxData data = {
            SDL_MESSAGEBOX_INFORMATION,
            nullptr,
            "Close project?",
            "You already have a project open. Do you want to close it?",
            2,
            buttons,
            nullptr
        };

        int button = 0;
        const auto result = SDL_ShowMessageBox(&data, &button);
        if (result < 0 || button == 0) {
            return;
        }

        closeProject(true);
    }

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
    if (m_openEditors.empty()) {
        m_activeEditor = editor;
    }
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

void ProjectManager::closeAllEditors() {
    for (const auto& editor : m_openEditors) {
        editor->notifyClosing();
    }

    m_openEditors.clear();
    m_activeEditor.reset();
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
            ImGui::Checkbox("Hide non SPL files", &m_hideOtherFiles);
            ImGui::InputText("Filter", &m_searchString);

            ImGui::BeginChild("##ProjectManagerFiles");

            for (const auto& entry : std::filesystem::directory_iterator(m_projectPath)) {
                if (entry.is_directory()) {
                    renderDirectory(entry.path());
                } else {
                    if (m_searchString.empty() || entry.path().string().contains(m_searchString)) {
                        renderFile(entry.path());
                    }
                }
            }

            ImGui::EndChild();
        }
    }

    ImGui::End();
}

void ProjectManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_DROPFILE: {
        const std::filesystem::path path = event.drop.file;
        if (std::filesystem::is_directory(path)) {
            openProject(path);
        } else if (path.extension() == ".spa") {
            openEditor(path);
        }
    } break;
    default:
        break;
    }
}

void ProjectManager::renderDirectory(const std::filesystem::path& path) {
    const auto text = fmt::format(ICON_FA_FOLDER " {}", path.filename().string());
    const bool open = ImGui::TreeNodeEx(text.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth);

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
                if (m_searchString.empty() || entry.path().string().contains(m_searchString)) {
                    renderFile(entry.path());
                }
            }
        }

        ImGui::TreePop();
    }
}

void ProjectManager::renderFile(const std::filesystem::path& path) {
    const auto text = fmt::format(ICON_FA_FILE " {}", path.filename().string());
    const bool isSplFile = path.extension().string() == ".spa";
    if (!isSplFile) {
        if (m_hideOtherFiles) {
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    ImGui::Indent(40.0f);
    if (ImGui::Selectable(text.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (isSplFile && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            openEditor(path);
        }
    }

    ImGui::Unindent(40.0f);

    if (!isSplFile) {
        ImGui::PopStyleColor();
        return;
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
