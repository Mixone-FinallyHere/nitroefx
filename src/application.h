#pragma once

#include "editor/editor.h"
#include "editor/project_manager.h"

#include <SDL_events.h>
#include <string_view>


class Application {
public:
    int run(int argc, char** argv);

private:
    void pollEvents();
    void dispatchEvent(const SDL_Event& event);
    void renderMenuBar();
    void setColors();

    static std::string_view openFile();
    static std::string_view saveFile();
    static std::string openProject();

private:
    bool m_running = true;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
};
