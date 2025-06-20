cmake_minimum_required(VERSION 3.15)

if (WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
endif()

project(nitroefx)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
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
    set(IMAGEQUANT_LIB "${LIBIMAGEQUANT_DIR}/target/release/libimagequant_sys.a")
endif()

add_custom_command(
    OUTPUT ${IMAGEQUANT_LIB}
    COMMAND cargo build --release --manifest-path ${LIBIMAGEQUANT_DIR}/imagequant-sys/Cargo.toml
    WORKING_DIRECTORY ${LIBIMAGEQUANT_DIR}
    COMMENT "Building imagequant_sys with cargo"
    VERBATIM
)

add_custom_target(libimagequant_sys_build ALL
    DEPENDS ${IMAGEQUANT_LIB}
)

add_dependencies(nitroefx libimagequant_sys_build)

target_include_directories(nitroefx PRIVATE 
    src 
    external/tinyfiledialogs
    external/stb
    external/libimagequant/imagequant-sys)
target_compile_definitions(nitroefx PRIVATE 
    GLM_ENABLE_EXPERIMENTAL
    IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(nitroefx PRIVATE 
    SDL3::SDL3 
    imgui::imgui 
    spdlog::spdlog
    fmt::fmt
    glm::glm
    OpenGL::GL
    GLEW::GLEW
    implot::implot
    ${IMAGEQUANT_LIB})

if (WIN32)
    target_link_libraries(nitroefx PRIVATE Ws2_32.lib ntdll.lib userenv.lib)
    target_compile_options(nitroefx PRIVATE /wd4312)
    target_sources(nitroefx PRIVATE data/icon.rc)
else()
    target_compile_options(nitroefx PRIVATE -Wno-int-to-pointer-cast -Wno-deprecated-enum-enum-conversion)
endif()
