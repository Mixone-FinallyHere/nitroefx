#pragma once

#include "editor_instance.h"

#include <SDL_events.h>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>


// Project is a bit of an overstatement here, because it's really just a directory
class ProjectManager {
public:
    void openProject(const std::filesystem::path& path);
    void closeProject(bool force = false);
    void openEditor(const std::filesystem::path& path);
    void openTempEditor(const std::filesystem::path& path);
    void closeEditor(const std::shared_ptr<EditorInstance>& editor, bool force = false);
    void closeTempEditor();
    void closeAllEditors();
    void saveAllEditors();

    void open();
    void render();

    std::span<const std::shared_ptr<EditorInstance>> getOpenEditors() const {
        return m_openEditors;
    }

    const std::shared_ptr<EditorInstance>& getActiveEditor() const {
        return m_activeEditor;
    }

    void setActiveEditor(const std::shared_ptr<EditorInstance>& editor) {
        m_activeEditor = editor;
    }

    const std::filesystem::path& getProjectPath() const {
        return m_projectPath;
    }

    bool hasProject() const {
        return !m_projectPath.empty();
    }

    bool hasOpenEditors() const {
        return !m_openEditors.empty();
    }

    bool hasActiveEditor() const {
        return m_activeEditor != nullptr;
    }

    void handleEvent(const SDL_Event& event);

    std::span<const std::shared_ptr<EditorInstance>> getUnsavedEditors() {
        return m_unsavedEditors;
    }

    void clearUnsavedEditors() {
        m_unsavedEditors.clear();
    }

private:
    void renderDirectory(const std::filesystem::path& path);
    void renderFile(const std::filesystem::path& path);

private:
    std::filesystem::path m_projectPath;

    std::vector<std::shared_ptr<EditorInstance>> m_openEditors;
    std::shared_ptr<EditorInstance> m_activeEditor;

    // Unsaved changes data
    std::vector<std::shared_ptr<EditorInstance>> m_unsavedEditors;

    bool m_open = true;
    bool m_hideOtherFiles = false;
    std::filesystem::path m_contextMenuPath;
    std::filesystem::path m_selectedFile;
    std::string m_searchString;
};


inline const auto g_projectManager = new ProjectManager();
