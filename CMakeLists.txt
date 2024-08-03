cmake_minimum_required(VERSION 3.15)

set(VCPKG_TARGET_TRIPLET x64-windows-static-md)

project(nitroefx)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(nitroefx ${SOURCES})

target_compile_definitions(nitroefx PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(nitroefx PRIVATE 
    SDL2::SDL2 
    SDL2::SDL2main 
    imgui::imgui 
    spdlog::spdlog_header_only 
    fmt::fmt-header-only 
    OpenGL::GL)
