#pragma once

#include "editor/editor.h"
#include "editor/project_manager.h"

#include <SDL3/SDL_events.h>
#include <string_view>


class Application {
public:
    Application();

    int run(int argc, char** argv);

    void saveConfig();
    ImFont* getFont(const std::string& name);

private:
    void pollEvents();
    void handleKeydown(const SDL_Event& event);
    void dispatchEvent(const SDL_Event& event);
    void renderMenuBar();
    void setColors();
    void loadFonts();
    void loadConfig();

    void addRecentFile(const std::string& path);
    void addRecentProject(const std::string& path);

    static std::filesystem::path getConfigPath();

    static std::string openFile();
    static std::string saveFile(const std::string& default_path = {});
    static std::string openProject();

private:
    bool m_running = true;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    std::unique_ptr<Editor> m_editor;

    std::deque<std::string> m_recentFiles;
    std::deque<std::string> m_recentProjects;

    std::map<std::string, ImFont*> m_fonts;
};

inline Application* g_application = nullptr;
