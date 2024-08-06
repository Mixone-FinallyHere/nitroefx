#include "application.h"
#include "spl/spl_archive.h"
#include "gl_texture.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <ShObjIdl.h>
#endif
#include <tinyfiledialogs.h>

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
    m_window = SDL_CreateWindow("NitroEFX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);
    if (m_window == nullptr) {
        spdlog::error("SDL_CreateWindow Error: {}", SDL_GetError());
        return 1;
    }

    m_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	setColors();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init("#version 450");

    while (m_running) {
        pollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

		renderMenuBar();
		g_projectManager->render();

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
		default:
			break;
		}

		dispatchEvent(event);
	}
}

void Application::dispatchEvent(const SDL_Event& event) {
}

void Application::renderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::BeginMenu("New...")) {
				if (ImGui::MenuItem("Project")) {
					spdlog::warn("New Project not implemented");
				}

				if (ImGui::MenuItem("SPL File")) {
					spdlog::warn("New SPL File not implemented");
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Open...")) {
				if (ImGui::MenuItem("Project")) {
					const auto path = openProject();
					if (!path.empty()) {
						g_projectManager->openProject(path);
					}
				}

				if (ImGui::MenuItem("SPL File")) {
					auto path = openFile();
					// TODO: Load the SPL file
				}

				ImGui::EndMenu();
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
	HRESULT_CHECK(dlg->Show(nullptr))

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
