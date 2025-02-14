#pragma once

#include <glm/glm.hpp>


class Renderer {
public:
    virtual void render(const glm::mat4& view, const glm::mat4& proj) = 0;
};
