cmake_minimum_required(VERSION 3.30)
include(ExternalProject)

if (WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
endif()

project(nitroefx)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(implot CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(nitroefx ${SOURCES} external/tinyfiledialogs/tinyfiledialogs.c)

set(LIBIMAGEQUANT_DIR ${CMAKE_SOURCE_DIR}/external/libimagequant)

if (WIN32 AND NOT CMAKE_BUILD_TYPE MATCHES Debug)
    message("Using WinMain for Windows builds")
    set_target_properties(nitroefx PROPERTIES WIN32_EXECUTABLE true)
endif()

if (WIN32)
    set(IMAGEQUANT_LIB "${LIBIMAGEQUANT_DIR}/target/release/imagequant_sys.lib")
else()
    set(IMAGEQUANT_LIB "${LIBIMAGEQUANT_DIR}/target/release/imagequant_sys.a")
endif()

ExternalProject_Add(
    libimagequant
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release
    INSTALL_COMMAND ""
    LOG_BUILD ON
    PREFIX ${LIBIMAGEQUANT_DIR}
)

ExternalProject_Add(
    libimagequant-sys
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release
    INSTALL_COMMAND ""
    LOG_BUILD ON
    BUILD_BYPRODUCTS ${IMAGEQUANT_LIB}
    PREFIX ${LIBIMAGEQUANT_DIR}/imagequant-sys
)

add_dependencies(nitroefx libimagequant libimagequant-sys)

target_include_directories(nitroefx PRIVATE 
    src 
    external/tinyfiledialogs
    external/stb
    external/libimagequant/imagequant-sys)
target_compile_definitions(nitroefx PRIVATE SDL_MAIN_HANDLED GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(nitroefx PRIVATE 
    SDL2::SDL2 
    SDL2::SDL2main 
    SDL2::SDL2-static
    imgui::imgui 
    spdlog::spdlog
    fmt::fmt
    glm::glm
    OpenGL::GL
    GLEW::GLEW
    implot::implot
    ${IMAGEQUANT_LIB})

if (WIN32)
    target_link_libraries(nitroefx PRIVATE
        Ws2_32.lib
        ntdll.lib
        userenv.lib
    )
endif()
