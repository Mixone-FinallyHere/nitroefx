#pragma once

#include "gfx/gl_util.h"
#include "gfx/gl_shader.h"
#include "types.h"

#include <glm/glm.hpp>
#include <vector>


class GridRenderer {
public:
    GridRenderer(const glm::ivec2& dimensions, const glm::vec2& spacing);

    void render(const glm::mat4& view, const glm::mat4& proj);

private:
    void createGrid();

private:
    glm::ivec2 m_dimensions;
    glm::vec2 m_spacing;
    std::vector<glm::vec3> m_vertices;

    GLShader m_shader;

    u32 m_vao = 0;
    u32 m_vbo = 0;

    u32 m_viewLocation = 0;
    u32 m_projLocation = 0;
};
