#include "application.h"
#include "fonts/IconsFontAwesome6.h"
#include "imgui/extensions.h"

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <ShObjIdl.h>
#endif
#include <tinyfiledialogs.h>

#define KEYBINDSTR(name) getKeybind(ApplicationAction::name)->toString().c_str()

static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        spdlog::error("OpenGL Error: {}", message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        spdlog::warn("OpenGL Error: {}", message);
        break;
    case GL_DEBUG_SEVERITY_LOW:
        spdlog::info("OpenGL Warn: {}", message);
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        spdlog::debug("OpenGL Info: {}", message);
        break;
    default:
        break;
    }
}

Application::Application() {
    if (g_application) {
        spdlog::error("Application already exists");
        throw std::runtime_error("Application already exists");
    }

    g_application = this;

    m_sortedActions = {
        ApplicationAction::OpenProject,
        ApplicationAction::OpenFile,
        ApplicationAction::Save,
        ApplicationAction::SaveAll,
        ApplicationAction::Close,
        ApplicationAction::CloseAll,
        ApplicationAction::Exit,
        ApplicationAction::Undo,
        ApplicationAction::Redo,
        ApplicationAction::PlayEmitter,
        ApplicationAction::PlayEmitterLooped,
        ApplicationAction::KillEmitters,
        ApplicationAction::ResetCamera,
    };

    m_modifierKeys = {
        SDLK_LCTRL, SDLK_RCTRL,
        SDLK_LSHIFT, SDLK_RSHIFT,
        SDLK_LALT, SDLK_RALT,
        SDLK_LGUI, SDLK_RGUI,
        SDLK_LMETA, SDLK_RMETA,
        SDLK_LHYPER, SDLK_RHYPER,
    };
}

int Application::run(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("SDL_Init Error: {}", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    constexpr auto windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    m_window = SDL_CreateWindow("NitroEFX", 1280, 720, windowFlags);
    if (m_window == nullptr) {
        spdlog::error("SDL_CreateWindow Error: {}", SDL_GetError());
        return 1;
    }

    m_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    glewExperimental = GL_TRUE;
    const GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        spdlog::error("GLEW Error: {}", (const char*)glewGetErrorString(glewError));
        return 1;
    }

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, nullptr);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    m_editor = std::make_unique<Editor>();
    m_settings = ApplicationSettings::getDefault();

    loadConfig();
    loadFonts();
    setColors();

    // loadConfig might change the window size so we create the window
    // in a hidden state and show it after loading the config
    SDL_ShowWindow(m_window);

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init("#version 450");

    m_preferencesWindowId = ImHashStr("Preferences##Application");

    if (argc > 1) {
        const std::filesystem::path arg = argv[1];
        if (std::filesystem::is_directory(arg)) {
            g_projectManager->openProject(arg);
        } else if (arg.extension() == ".spl") {
            g_projectManager->openEditor(arg);
        } else {
            spdlog::warn("Invalid argument: {}", arg.string());
        }
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame = std::chrono::high_resolution_clock::now();

    while (m_running) {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto delta = std::chrono::duration<float>(now - lastFrame).count();
        m_deltaTime = delta;

        pollEvents();

        m_editor->updateParticles(delta);
        m_editor->renderParticles();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        renderMenuBar();
        g_projectManager->render();
        m_editor->render();

        if (m_preferencesOpen) {
            const auto center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });

            renderPreferences();
        }

        if (m_performanceWindowOpen) {
            renderPerformanceWindow();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window* currentWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext currentContext = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(currentWindow, currentContext);
        }

        SDL_GL_SwapWindow(m_window);
        lastFrame = now;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    saveConfig();

    return 0;
}

void Application::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_EVENT_QUIT:
            m_running = false;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == SDL_GetWindowID(m_window)) {
                m_running = false;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            saveConfig(); // Save the window size
            break;

        case SDL_EVENT_KEY_DOWN:
            handleKeydown(event);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            handleMouseDown(event);
            break;

        default:
            break;
        }

        dispatchEvent(event);
    }
}

void Application::handleKeydown(const SDL_Event& event) {
    const auto io = ImGui::GetIO();
    if (io.WantTextInput) {
        return;
    }

    if (m_listeningForInput) {
        if (m_modifierKeys.contains(event.key.key)) {
            return; // Ignore modifier keys
        }

        if (event.key.key == SDLK_ESCAPE) {
            m_listeningForInput = false;
            m_listeningKeybind = nullptr;
            m_exitKeybindListening = true;
            return;
        }

        m_listeningKeybind->type = KeybindType::Key;
        m_listeningKeybind->key = event.key.key;
        m_listeningKeybind->modifiers = event.key.mod;
        m_listeningForInput = false;
        m_listeningKeybind = nullptr;
        m_exitKeybindListening = true;
        return;
    }

    for (const auto& [action, keybind] : m_settings.keybinds) {
        if (keybind.type == KeybindType::Key) {
            if (event.key.key == keybind.key && event.key.mod == keybind.modifiers) {
                executeAction(action);
                return;
            }
        }
    }
    
    switch (event.key.key) {
    case SDLK_N:
        if (event.key.mod & SDL_KMOD_CTRL) {
            if (event.key.mod & SDL_KMOD_SHIFT) {
                spdlog::warn("New Project not implemented");
            } else {
                spdlog::warn("New SPL File not implemented");
            }
        }
        break;

    case SDLK_O:
        if (event.key.mod & SDL_KMOD_CTRL) {
            if (event.key.mod & SDL_KMOD_SHIFT) {
                const auto path = openProject();
                if (!path.empty()) {
                    g_projectManager->openProject(path);
                }
            } else {
                const auto path = openFile();
                if (!path.empty()) {
                    g_projectManager->openEditor(path);
                }
            }
        }
        
        break;

    case SDLK_S:
        if (event.key.mod & SDL_KMOD_CTRL) {
            if (event.key.mod & SDL_KMOD_SHIFT) {
                g_projectManager->saveAllEditors();
            } else {
                m_editor->save();
            }
        }
        break;

    case SDLK_W:
        if (event.key.mod & SDL_KMOD_CTRL) {
            if (event.key.mod & SDL_KMOD_SHIFT) {
                if (g_projectManager->hasOpenEditors()) {
                    g_projectManager->closeAllEditors();
                }
            } else {
                if (g_projectManager->hasActiveEditor()) {
                    g_projectManager->closeEditor(g_projectManager->getActiveEditor());
                }
            }
        }
        break;

    case SDLK_P:
        if (event.key.mod & SDL_KMOD_CTRL) {
            if (event.key.mod & SDL_KMOD_SHIFT) {
                m_editor->playEmitterAction(EmitterSpawnType::Looped);
            } else {
                m_editor->playEmitterAction(EmitterSpawnType::SingleShot);
            }
        }
        break;

    case SDLK_K:
        if (event.key.mod & SDL_KMOD_CTRL) {
            m_editor->killEmitters();
        }
        break;
    
    case SDLK_R:
        if (event.key.mod & SDL_KMOD_CTRL) {
            m_editor->resetCamera();
        }
        break;

    case SDLK_Z:
        if (event.key.mod & SDL_KMOD_CTRL) {
            m_editor->undo();
        }
        break;

    case SDLK_Y:
        if (event.key.mod & SDL_KMOD_CTRL) {
            m_editor->redo();
        }
        break;

    case SDLK_F4:
        if (event.key.mod & SDL_KMOD_ALT) {
            m_running = false;
        }
        break;

    default:
        break;
    }
}

void Application::handleMouseDown(const SDL_Event& event) {
    if (m_listeningForInput) {
        if (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT) {
            return; // Don't allow remapping left/right mouse buttons
        }

        m_listeningKeybind->type = KeybindType::Mouse;
        m_listeningKeybind->button = event.button.button;
        m_listeningForInput = false;
        m_listeningKeybind = nullptr;
        m_exitKeybindListening = true;
        return;
    }

    for (const auto& [action, keybind] : m_settings.keybinds) {
        if (keybind.type == KeybindType::Mouse) {
            if (event.button.button == keybind.button) {
                executeAction(action);
                return;
            }
        }
    }
}

void Application::dispatchEvent(const SDL_Event& event) {
    g_projectManager->handleEvent(event);
    m_editor->handleEvent(event);
}

void Application::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        const bool hasProject = g_projectManager->hasProject();
        const bool hasActiveEditor = g_projectManager->hasActiveEditor();
        const bool hasOpenEditors = g_projectManager->hasOpenEditors();

        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("New")) {
                if (ImGui::MenuItemIcon(ICON_FA_FOLDER_PLUS, "Project", "Ctrl+Shift+N")) {
                    spdlog::warn("New Project not implemented");
                }

                if (ImGui::MenuItemIcon(ICON_FA_FILE_CIRCLE_PLUS, "SPL File", "Ctrl+N")) {
                    spdlog::warn("New SPL File not implemented");
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Open")) {
                if (ImGui::MenuItemIcon(ICON_FA_FOLDER, "Project", KEYBINDSTR(OpenProject))) {
                    const auto path = openProject();
                    if (!path.empty()) {
                        addRecentProject(path);
                        g_projectManager->openProject(path);
                    }
                }

                if (ImGui::MenuItemIcon(ICON_FA_FILE, "SPL File", KEYBINDSTR(OpenFile))) {
                    const auto path = openFile();
                    if (!path.empty()) {
                        addRecentFile(path);
                        g_projectManager->openEditor(path);
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Open Recent")) {
                ImGui::SeparatorText("Projects");
                if (m_recentProjects.empty()) {
                    ImGui::MenuItem("No Recent Projects", nullptr, false, false);
                }

                for (const auto& path : m_recentProjects) {
                    if (ImGui::MenuItem(path.c_str())) {
                        g_projectManager->openProject(path);
                    }
                }

                ImGui::SeparatorText("Files");
                if (m_recentFiles.empty()) {
                    ImGui::MenuItem("No Recent Files", nullptr, false, false);
                }

                for (const auto& path : m_recentFiles) {
                    if (ImGui::MenuItem(path.c_str())) {
                        g_projectManager->openEditor(path);
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItemIcon(ICON_FA_FLOPPY_DISK, "Save", KEYBINDSTR(Save), false, hasActiveEditor)) {
                m_editor->save();
            }

            if (ImGui::MenuItemIcon(ICON_FA_FLOPPY_DISK, "Save As...", nullptr, false, hasActiveEditor)) {
                const auto path = saveFile();
                if (!path.empty()) {
                    m_editor->saveAs(path);
                    addRecentFile(path);
                }
            }

            if (ImGui::MenuItemIcon(ICON_FA_FLOPPY_DISK, "Save All", KEYBINDSTR(SaveAll), false, hasOpenEditors)) {
                g_projectManager->saveAllEditors();
            }

            if (ImGui::MenuItemIcon(ICON_FA_XMARK, "Close", KEYBINDSTR(Close), false, hasActiveEditor)) {
                g_projectManager->closeEditor(g_projectManager->getActiveEditor());
            }

            if (ImGui::MenuItemIcon(ICON_FA_XMARK, "Close All", KEYBINDSTR(CloseAll), false, hasOpenEditors)) {
                g_projectManager->closeAllEditors();
            }

            if (ImGui::MenuItemIcon(ICON_FA_XMARK, "Close Project", nullptr, false, hasProject)) {
                g_projectManager->closeProject();
            }

            if (ImGui::MenuItemIcon(ICON_FA_POWER_OFF, "Exit", KEYBINDSTR(Exit))) {
                m_running = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItemIcon(ICON_FA_ROTATE_LEFT, "Undo", KEYBINDSTR(Undo), false, m_editor->canUndo())) {
                m_editor->undo();
            }

            if (ImGui::MenuItemIcon(ICON_FA_ROTATE_RIGHT, "Redo", KEYBINDSTR(Redo), false, m_editor->canRedo())) {
                m_editor->redo();
            }

            if (ImGui::MenuItemIcon(ICON_FA_PLAY, "Play Emitter", KEYBINDSTR(PlayEmitter), false, hasActiveEditor)) {
                m_editor->playEmitterAction(EmitterSpawnType::SingleShot);
            }

            if (ImGui::MenuItemIcon(ICON_FA_REPEAT, "Play Looped Emitter", KEYBINDSTR(PlayEmitterLooped), false, hasActiveEditor)) {
                m_editor->playEmitterAction(EmitterSpawnType::Looped);
            }

            if (ImGui::MenuItem("Kill Emitters", KEYBINDSTR(KillEmitters), false, hasActiveEditor)) {
                m_editor->killEmitters();
            }

            if (ImGui::MenuItem("Reset Camera", KEYBINDSTR(ResetCamera), false, hasActiveEditor)) {
                m_editor->resetCamera();
            }

            if (ImGui::MenuItemIcon(ICON_FA_WRENCH, "Preferences")) {
                m_preferencesOpen = true;

                ImGui::PushOverrideID(m_preferencesWindowId);
                ImGui::OpenPopup("Preferences##Application");
                ImGui::PopID();
            }

            m_editor->renderMenu("Edit");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItemIcon(ICON_FA_FOLDER_TREE, "Project Manager")) {
                g_projectManager->open();
            }

            if (ImGui::MenuItemIcon(ICON_FA_WRENCH, "Resource Picker")) {
                m_editor->openPicker();
            }

            if (ImGui::MenuItemIcon(ICON_FA_IMAGES, "Texture Manager")) {
                m_editor->openTextureManager();
            }

            if (ImGui::MenuItemIcon(ICON_FA_SLIDERS, "Resource Editor")) {
                m_editor->openEditor();
            }

            ImGui::MenuItemIcon(ICON_FA_GAUGE, "Performance", nullptr, &m_performanceWindowOpen);

            m_editor->renderMenu("View");

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Application::renderPreferences() {
    ImGui::PushOverrideID(m_preferencesWindowId);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

    if (ImGui::BeginPopupModal("Preferences##Application", &m_preferencesOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SeparatorText("Keybinds");

        if (ImGui::BeginTable("Keybinds##Application", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersH)) {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Keybind", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            
            for (auto action : m_sortedActions) {
                auto& keybind = m_settings.keybinds[action];

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(ApplicationAction::Names.at(action));
                
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(300);

                constexpr auto flags = ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_NoAutoClosePopups;
                if (ImGui::Selectable(keybind.toString().c_str(), false, flags)) {
                    m_listeningForInput = true;
                    m_listeningKeybind = &keybind;
                    ImGui::OpenPopup("Keybind##Application");
                }
            }

            if (m_listeningForInput) {
                ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
                // Place the popup in the center of the window
                const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize({ 350, 200 }, ImGuiCond_Always);

                constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;
                if (ImGui::BeginPopupModal("Keybind##Application", nullptr, flags)) {
                    const auto drawList = ImGui::GetWindowDrawList();

                    const auto windowPos = ImGui::GetWindowPos();
                    const auto windowSize = ImGui::GetWindowSize();
                    const auto textSize = ImGui::CalcTextSize("Press any key or button to bind");

                    const auto textPos = windowPos + ImVec2((windowSize.x - textSize.x) / 2, (windowSize.y - textSize.y) / 2);
                    drawList->AddText(
                        textPos, 
                        ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 
                        "Press any key or button to bind"
                    );

                    if (m_exitKeybindListening) {
                        ImGui::CloseCurrentPopup();
                        m_exitKeybindListening = false;
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleVar();
            }

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

void Application::renderPerformanceWindow() {
    if (ImGui::Begin("Performance", &m_performanceWindowOpen)) {
        ImGui::SeparatorText("Application");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Delta Time: %.3f ms", m_deltaTime * 1000.0f);
        ImGui::Text("Frame Time: %.3f ms", ImGui::GetIO().DeltaTime * 1000.0f);

        ImGui::SeparatorText("Current Editor");
        m_editor->renderStats();
    }

    ImGui::End();
}

void Application::setColors() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 2.0f;
    style.PopupRounding = 2.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(11.0f, 4.0f);
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 16.0f;
    style.ScrollbarRounding = 2.4f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 2.2f;
    style.TabRounding = 2.0f;
    style.TabBorderSize = 0.0f;
    style.TabCloseButtonMinWidthSelected = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.8369014859199524f, 0.8369098901748657f, 0.8369098901748657f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1411764770746231f, 0.1607843190431595f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1142772808670998f, 0.1279543489217758f, 0.1416308879852295f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.2354436367750168f, 0.2712275087833405f, 0.3304721117019653f, 0.4470588266849518f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1607843190431595f, 0.1803921610116959f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1775497645139694f, 0.198217362165451f, 0.2188841104507446f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1976459473371506f, 0.2232870012521744f, 0.2489270567893982f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1153456568717957f, 0.1187514066696167f, 0.1330472230911255f, 0.5299999713897705f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5176470875740051f, 0.3568627536296844f, 0.6705882549285889f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5157450437545776f, 0.3568627536296844f, 0.6705882549285889f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5788434743881226f, 0.2895798087120056f, 0.8540772199630737f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2356939762830734f, 0.270569235086441f, 0.3098039329051971f, 0.7843137383460999f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3462888300418854f, 0.2819724082946777f, 0.3690987229347229f, 0.7843137383460999f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.3874832093715668f, 0.2803691029548645f, 0.4039215743541718f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1411764770746231f, 0.1607843190431595f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1607843190431595f, 0.1803921610116959f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.1904621571302414f, 0.2132570743560791f, 0.2360514998435974f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.2477850168943405f, 0.248460590839386f, 0.3261802792549133f, 0.7799999713897705f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3376374840736389f, 0.346673846244812f, 0.4034335017204285f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.3162703216075897f, 0.3700733184814453f, 0.4334763884544373f, 0.09019608050584793f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.9999899864196777f, 0.9999945759773254f, 1.0f, 0.6700000166893005f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3372549116611481f, 0.3450980484485626f, 0.4039215743541718f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1559063494205475f, 0.1766677051782608f, 0.1974248886108398f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.1727974563837051f, 0.2001329809427261f, 0.2274678349494934f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1215686276555061f, 0.1411764770746231f, 0.1607843190431595f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.153235450387001f, 0.177476242184639f, 0.2017167210578918f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.5814036130905151f, 0.1259923875331879f, 0.8154506683349609f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.6688579916954041f, 0.211847722530365f, 0.9313304424285889f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1411764770746231f, 0.1607843190431595f, 0.1803921610116959f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3093271851539612f, 0.3093271851539612f, 0.3490196168422699f, 0.4980392158031464f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2276816666126251f, 0.2276816666126251f, 0.2470588237047195f, 0.4980392158031464f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.9764705896377563f, 0.9117898344993591f, 0.3499999940395355f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.5249696969985962f, 0.3654515743255615f, 0.6652360558509827f, 0.8999999761581421f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.5275201797485352f, 0.3633987307548523f, 0.6666666865348816f, 0.8352941274642944f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(7.167382136685774e-07f, 7.775358312755998e-07f, 9.999999974752427e-07f, 0.3499999940395355f);
}

#include "fonts/tahoma_font.h"
#include "fonts/tahoma_italic_font.h"
#include "fonts/icon_font.h"

void Application::loadFonts() {
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = true;
    config.FontDataOwnedByAtlas = false;

    io.Fonts->AddFontFromMemoryCompressedTTF(
        g_tahoma_compressed_data, 
        g_tahoma_compressed_size, 
        18.0f, 
        &config
    );

    config.MergeMode = true;
    constexpr ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    io.Fonts->AddFontFromMemoryCompressedTTF(
        g_icon_font_compressed_data, 
        g_icon_font_compressed_size, 
        18.0f, 
        &config,
        iconRanges
    );

    config.MergeMode = false;

    m_fonts["Italic"] = io.Fonts->AddFontFromMemoryCompressedTTF(
        g_tahoma_italic_compressed_data,
        g_tahoma_italic_compressed_size,
        18.0f,
        &config
    );

    io.Fonts->Build();
}

void Application::loadConfig() {
    const auto configPath = getConfigPath();
    if (!std::filesystem::exists(configPath)) {
        spdlog::info("Config path does not exist, creating: {}", configPath.string());
        std::filesystem::create_directories(configPath);
    }

    const auto configFile = configPath / "config.json";
    if (!std::filesystem::exists(configFile)) {
        spdlog::info("Config file does not exist, creating: {}", configFile.string());
        nlohmann::json config;
        config["recentFiles"] = nlohmann::json::array();
        config["recentProjects"] = nlohmann::json::array();
        std::ofstream outFile(configFile);

        outFile << config.dump(4);
    }

    std::ifstream inFile(configFile);
    if (!inFile) {
        spdlog::error("Failed to open config file: {}", configFile.string());
        return;
    }

    nlohmann::json config = nlohmann::json::object();
    try {
        inFile >> config;
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Failed to parse config file: {}", e.what());
        return;
    }

    for (const auto& file : config["recentFiles"]) {
        m_recentFiles.push_back(file.get<std::string>());
    }

    for (const auto& project : config["recentProjects"]) {
        m_recentProjects.push_back(project.get<std::string>());
    }

    if (config.contains("keybinds")) {
        for (const auto& keybind : config["keybinds"]) {
            Keybind bind{};
            bind.type = static_cast<KeybindType>(keybind["type"].get<int>());
            if (bind.type == KeybindType::Key) {
                bind.key = keybind.value<SDL_Keycode>("key", SDLK_UNKNOWN);
                bind.modifiers = keybind.value<SDL_Keymod>("modifiers", 0);
            }
            else if (bind.type == KeybindType::Mouse) {
                bind.button = keybind.value<Uint8>("button", SDL_BUTTON_X1);
            }

            m_settings.keybinds[keybind["id"].get<u32>()] = bind;
        }
    }

    if (config.contains("windowPos")) {
        const auto& pos = config["windowPos"];
        SDL_SetWindowPosition(
            m_window,
            pos.value<int>("x", SDL_WINDOWPOS_CENTERED),
            pos.value<int>("y", SDL_WINDOWPOS_CENTERED)
        );
    }

    if (config.contains("windowSize")) {
        const auto& size = config["windowSize"];
        if (size.value("maximized", false)) {
            SDL_MaximizeWindow(m_window);
        } else {
            SDL_SetWindowSize(m_window, size["w"].get<int>(), size["h"].get<int>());
        }
    }

    m_editor->loadConfig(config);
}

void Application::executeAction(u32 action) {
    spdlog::info("Executing Action: {}", ApplicationAction::Names.at(action));

    switch (action) {
    case ApplicationAction::OpenProject: {
        const auto projectPath = openProject();
        if (!projectPath.empty()) {
            addRecentProject(projectPath);
            g_projectManager->openProject(projectPath);
        }
    } break;
    case ApplicationAction::OpenFile: {
        const auto filePath = openFile();
        if (!filePath.empty()) {
            addRecentFile(filePath);
            g_projectManager->openEditor(filePath);
        }
    } break;
    case ApplicationAction::Save:
        m_editor->save();
        break;
    case ApplicationAction::SaveAll:
        g_projectManager->saveAllEditors();
        break;
    case ApplicationAction::Close:
        if (g_projectManager->hasActiveEditor()) {
            g_projectManager->closeEditor(g_projectManager->getActiveEditor());
        }
        break;
    case ApplicationAction::CloseAll:
        if (g_projectManager->hasOpenEditors()) {
            g_projectManager->closeAllEditors();
        }
        break;
    case ApplicationAction::Exit:
        m_running = false;
        break;
    case ApplicationAction::PlayEmitter:
        m_editor->playEmitterAction(EmitterSpawnType::SingleShot);
        break;
    case ApplicationAction::PlayEmitterLooped:
        m_editor->playEmitterAction(EmitterSpawnType::Looped);
        break;
    case ApplicationAction::KillEmitters:
        m_editor->killEmitters();
        break;
    case ApplicationAction::ResetCamera:
        m_editor->resetCamera();
        break;
    }
}

void Application::saveConfig() {
    const auto configPath = getConfigPath();
    if (!std::filesystem::exists(configPath)) {
        spdlog::info("Config path does not exist, creating: {}", configPath.string());
        std::filesystem::create_directories(configPath);
    }

    const auto configFile = configPath / "config.json";
    nlohmann::json config;

    for (const auto& file : m_recentFiles) {
        config["recentFiles"].push_back(file);
    }

    for (const auto& project : m_recentProjects) {
        config["recentProjects"].push_back(project);
    }

    auto keybinds = nlohmann::json::array();
    for (const auto& keybind : m_settings.keybinds) {
        auto b = nlohmann::json::object();
        b["id"] = keybind.first;
        b["type"] = keybind.second.type;
        if (keybind.second.type == KeybindType::Key) {
            b["key"] = keybind.second.key;
            b["modifiers"] = keybind.second.modifiers;
        } else if (keybind.second.type == KeybindType::Mouse) {
            b["button"] = keybind.second.button;
        }

        keybinds.push_back(b);
    }

    config["keybinds"] = keybinds;

    int x, y;
    SDL_GetWindowPosition(m_window, &x, &y);
    config["windowPos"] = {
        { "x", x },
        { "y", y }
    };

    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    config["windowSize"] = {
        { "w", width },
        { "h", height },
        { "maximized", !!(SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) }
    };
    
    m_editor->saveConfig(config);

    std::ofstream outFile(configFile);
    if (!outFile) {
        spdlog::error("Failed to open config file for writing: {}", configFile.string());
        return;
    }
    
    try {
        outFile << config.dump(4);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to write config file: {}", e.what());
        return;
    }
}

ImFont* Application::getFont(const std::string& name) {
    const auto it = m_fonts.find(name);
    return it != m_fonts.end() ? it->second : nullptr;
}

std::optional<Keybind> Application::getKeybind(u32 action) const {
    if (!m_settings.keybinds.contains(action)) {
        return std::nullopt;
    }

    return m_settings.keybinds.at(action);
}

std::optional<Keybind> Application::getKeybind(const std::string_view& name) const {
    return getKeybind(crc::crc32(name.data(), name.size()));
}

void Application::addRecentFile(const std::string& path) {
    if (m_recentFiles.size() >= 10) {
        m_recentFiles.pop_back();
    }

    m_recentFiles.push_front(path);

    saveConfig();
}

void Application::addRecentProject(const std::string& path) {
    if (m_recentProjects.size() >= 10) {
        m_recentProjects.pop_back();
    }

    m_recentProjects.push_front(path);

    saveConfig();
}

std::filesystem::path Application::getConfigPath() {
#ifdef _WIN32
    char* buffer;
    size_t size;
    if (_dupenv_s(&buffer, &size, "APPDATA") != 0 || !buffer) {
        spdlog::error("Failed to get APPDATA environment variable");
        return {};
    }

    std::filesystem::path configPath = std::filesystem::path(buffer) / "nitroefx";
    free(buffer);

    return configPath;
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig) {
        return std::filesystem::path(xdgConfig) / "nitroefx";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / ".config" / "nitroefx";
        } else {
            spdlog::error("Failed to get XDG_CONFIG_HOME or HOME environment variable");
            return {};
        }
    }
#endif
}

std::string Application::openFile() {
    const char* filters[] = { "*.spa" };
    const char* result = tinyfd_openFileDialog(
        "Open File", 
        "", 
        1, 
        filters, 
        "SPL Files", 
        false
    );

    return result ? result : "";
}

std::string Application::saveFile(const std::string& default_path) {
    const char* filters[] = { "*.spa" };
    const char* result = tinyfd_saveFileDialog(
        "Save File",
        default_path.c_str(),
        1,
        filters,
        "SPL Files"
    );

    return result ? result : "";
}

std::string Application::openProject() {
#ifdef _WIN32
#ifdef _DEBUG
#define HRESULT_CHECK(hr) if (FAILED(hr)) { spdlog::error("HRESULT failed @ {}:{}", __FILE__, __LINE__); return ""; }
#else
#define HRESULT_CHECK(hr) if (FAILED(hr)) { spdlog::error("operation failed: {}", hr); return ""; }
#endif

    // Implementing this manually because tinyfiledialog uses the old SHBrowseForFolder API (which sucks)
    IFileOpenDialog* dlg;

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        spdlog::error("Failed to initialize COM");
        return "";
    }

    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, 
        IID_IFileOpenDialog, reinterpret_cast<void**>(&dlg)))) {
        spdlog::error("Failed to create file open dialog");
        return "";
    }

    HRESULT_CHECK(dlg->SetTitle(L"Open Project"))
    HRESULT_CHECK(dlg->SetOptions(FOS_PICKFOLDERS | FOS_PATHMUSTEXIST))
    if (dlg->Show(nullptr) != S_OK) {
        spdlog::info("User cancelled dialog");
        return "";
    }

    IShellItem* item;
    HRESULT_CHECK(dlg->GetResult(&item))
    PWSTR path;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
        spdlog::error("Failed to get path");
        return "";
    }

    return tinyfd_utf16to8(path);
#else
    const auto folder = tinyfd_selectFolderDialog("Open Project", nullptr);
    return folder ? folder : "";
#endif
}
