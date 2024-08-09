#pragma once

#include <gl/glew.h>
#include <spdlog/spdlog.h>


#ifdef _DEBUG

#define glCall(x) do { x; if (const auto error = glGetError()) { \
    spdlog::error("OpenGL error at {}:{} -> {}", __FILE__, __LINE__, error);\
}} while (0)

#else

#define glCall(x) x

#endif
