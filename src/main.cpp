#include "application.h"

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>


int nitroefx_main(int argc, char** argv) {
    // -- GUI Mode
    // $ nitroefx
    // $ nitroefx <path>

    // -- CLI Mode
    // $ nitroefx cli <options>

    argparse::ArgumentParser program("nitroefx");
    
    // Optional path for GUI mode
    program.add_argument("path")
           .help("Optional path for GUI mode")
           .nargs(0);

    // Subcommand for CLI
    argparse::ArgumentParser cli("cli", "Command Line Interface for nitroefx");
    cli.add_argument("path").help("Path to a .spa file").required().nargs(1);
    cli.add_argument("-e", "--export").help("Export textures").default_value(false).implicit_value(true);
    cli.add_argument("-i", "--index").help("Texture index to export").nargs(argparse::nargs_pattern::at_least_one).scan<'i', int>();
    cli.add_argument("-f", "--format").help("Export format (png, bmp, tga). Default is png").nargs(1);
    cli.add_argument("-o", "--output").help("Output path."
        " Can be a directory (always) or a file path (only when used with a single index -i)").nargs(1);

    program.add_subparser(cli);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        spdlog::error("Error parsing arguments: {}", err.what());
        std::exit(1);
    }

    Application app;
    if (program.is_subcommand_used("cli")) {
        return app.runCli(cli);
    } else {
        return app.run(argc, argv);
    }
}

