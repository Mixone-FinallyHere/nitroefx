#include "application.h"
#include "fonts/IconsFontAwesome6.h"

#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <SDL2/SDL_opengl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <ShObjIdl.h>
#endif
#include <tinyfiledialogs.h>


static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		spdlog::critical("OpenGL Error: {}", message);
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		spdlog::warn("OpenGL Error: {}", message);
		break;
	case GL_DEBUG_SEVERITY_LOW:
		spdlog::info("OpenGL Error: {}", message);
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		spdlog::debug("OpenGL Error: {}", message);
		break;
	default:
		break;
	}
}

int Application::run(int argc, char** argv) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    constexpr auto windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    m_window = SDL_CreateWindow("NitroEFX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, windowFlags);
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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	loadFonts();
	setColors();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init("#version 450");

	m_editor = std::make_unique<Editor>();
	std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame = std::chrono::high_resolution_clock::now();

    while (m_running) {
		const auto now = std::chrono::high_resolution_clock::now();
		const auto delta = std::chrono::duration<float>(now - lastFrame).count();
        pollEvents();

		m_editor->updateParticles(delta);
		m_editor->renderParticles();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

		renderMenuBar();
		g_projectManager->render();
		m_editor->render();

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

    return 0;
}

void Application::pollEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			m_running = false;
			break;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_CLOSE 
				&& event.window.windowID == SDL_GetWindowID(m_window)) {
				m_running = false;
			}
			break;

		case SDL_KEYDOWN:
			handleKeydown(event);
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

	switch (event.key.keysym.sym) {
	case SDLK_n:
		if (event.key.keysym.mod & KMOD_CTRL) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
				spdlog::warn("New Project not implemented");
			} else {
				spdlog::warn("New SPL File not implemented");
			}
		}
		break;

	case SDLK_o:
		if (event.key.keysym.mod & KMOD_CTRL) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
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

	case SDLK_s:
		if (event.key.keysym.mod & KMOD_CTRL) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
				spdlog::warn("Save All not implemented");
			} else {
				spdlog::warn("Save not implemented");
			}
		}
		break;

	case SDLK_w:
		if (event.key.keysym.mod & KMOD_CTRL) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
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

	case SDLK_p:
		if (event.key.keysym.mod & KMOD_CTRL) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
				m_editor->playEmitterAction(EmitterSpawnType::Looped);
			} else {
				m_editor->playEmitterAction(EmitterSpawnType::SingleShot);
			}
		}
		break;

	case SDLK_k:
		if (event.key.keysym.mod & KMOD_CTRL) {
			m_editor->killEmitters();
		}
		break;

	case SDLK_F4:
		if (event.key.keysym.mod & KMOD_ALT) {
			m_running = false;
		}
		break;

	default:
		break;
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
				if (ImGui::MenuItem(ICON_FA_FOLDER_PLUS " Project", "Ctrl+Shift+N")) {
					spdlog::warn("New Project not implemented");
				}

				if (ImGui::MenuItem(ICON_FA_FILE_CIRCLE_PLUS " SPL File", "Ctrl+N")) {
					spdlog::warn("New SPL File not implemented");
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Open")) {
				if (ImGui::MenuItem(ICON_FA_FOLDER " Project", "Ctrl+Shift+O")) {
					const auto path = openProject();
					if (!path.empty()) {
						g_projectManager->openProject(path);
					}
				}

				if (ImGui::MenuItem(ICON_FA_FILE " SPL File", "Ctrl+O")) {
					const auto path = openFile();
					if (!path.empty()) {
						g_projectManager->openEditor(path);
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save", "Ctrl+S", false, hasActiveEditor)) {
				spdlog::warn("Save not implemented");
			}

			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As...", nullptr, false, hasActiveEditor)) {
				spdlog::warn("Save As not implemented");
			}

			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save All", "Ctrl+Shift+S", false, hasOpenEditors)) {
				spdlog::warn("Save All not implemented");
			}

			if (ImGui::MenuItem(ICON_FA_XMARK " Close", "Ctrl+W", false, hasActiveEditor)) {
				g_projectManager->closeEditor(g_projectManager->getActiveEditor());
			}

			if (ImGui::MenuItem(ICON_FA_XMARK " Close All", "Ctrl+Shift+W", false, hasOpenEditors)) {
				g_projectManager->closeAllEditors();
			}

			if (ImGui::MenuItem(ICON_FA_XMARK " Close Project", nullptr, false, hasProject)) {
				g_projectManager->closeProject();
			}

			if (ImGui::MenuItem(ICON_FA_POWER_OFF " Exit", "Alt+F4")) {
				m_running = false;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
				spdlog::warn("Undo not implemented");
			}

			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
				spdlog::warn("Redo not implemented");
			}

			if (ImGui::MenuItem("Play Single Shot Emitter", "Ctrl+P", false, hasActiveEditor)) {
				m_editor->playEmitterAction(EmitterSpawnType::SingleShot);
			}

			if (ImGui::MenuItem("Play Looped Emitter", "Ctrl+Shift+P", false, hasActiveEditor)) {
				m_editor->playEmitterAction(EmitterSpawnType::Looped);
			}

			if (ImGui::MenuItem("Kill Emitters", "Ctrl+K", false, hasActiveEditor)) {
				m_editor->killEmitters();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Project Manager")) {
				g_projectManager->open();
			}

			if (ImGui::MenuItem("Resource Picker")) {
				m_editor->openPicker();
			}

			if (ImGui::MenuItem("Resource Editor")) {
				m_editor->openEditor();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
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
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(11.0f, 4.0f);
	style.FrameRounding = 2.2f;
	style.FrameBorderSize = 0.0f;
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
	style.TabMinWidthForCloseButton = 0.0f;
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

	io.Fonts->Build();
}

std::string_view Application::openFile() {
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

std::string_view Application::saveFile() {
	return {};
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
