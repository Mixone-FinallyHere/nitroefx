
#include <tinyfiledialogs.h>

#include <string>
#include <vector>


extern int nitroefx_main(int argc, char** argv);


#if defined(_WIN32) && !defined(_DEBUG) // Windows GUI application entry point

#include <Windows.h>
#include <shellapi.h>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd) {
    int argc = 0;
    const auto wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Convert wargv to utf-8
    std::vector<char*> argv(argc);
    std::vector<std::string> argvs(argc);

    for (int i = 0; i < argc; i++) {
        argvs[i] = tinyfd_utf16to8(wargv[i]);
        argv[i] = argvs[i].data();
    }

    const int result = nitroefx_main(argc, argv.data());

    LocalFree(wargv);
    return result;
}

#else // Regular entry point

int main(int argc, char** argv) {
    return nitroefx_main(argc, argv);
}

#endif

