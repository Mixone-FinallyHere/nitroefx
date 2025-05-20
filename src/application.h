#pragma once

#include "application_settings.h"
#include "editor/editor.h"
#include "editor/project_manager.h"

#include <SDL3/SDL_events.h>
#include <string_view>

#include <optional>


class Application {
public:
    Application();

    int run(int argc, char** argv);

    void saveConfig();
    ImFont* getFont(const std::string& name);

    std::optional<Keybind> getKeybind(u32 action) const;
    std::optional<Keybind> getKeybind(const std::string_view& name) const;

private:
    void pollEvents();
    void handleKeydown(const SDL_Event& event);
    void dispatchEvent(const SDL_Event& event);
    void renderMenuBar();
    void renderPreferences();
    void setColors();
    void loadFonts();
    void loadConfig();
    void executeAction(u32 action);

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
    
    ApplicationSettings m_settings;
    int m_preferencesWindowId = 0;
    bool m_preferencesOpen = false;
    bool m_listeningForInput = false;
    bool m_exitKeybindListening = false;
    Keybind* m_listeningKeybind = nullptr;
};

inline Application* g_application = nullptr;
