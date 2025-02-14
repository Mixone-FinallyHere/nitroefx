#pragma once

#include "renderer.h"
#include "gfx/gl_util.h"
#include "gfx/gl_shader.h"
#include "types.h"

#include <glm/glm.hpp>
#include <vector>


class DebugRenderer : public Renderer {
public:
    DebugRenderer(u32 maxLines);

    void render(const glm::mat4& view, const glm::mat4& proj) override;

    void addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color = glm::vec3(1.0f));
    void addPlane(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& color = glm::vec3(1.0f));
    void addBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
    // void addSphere(const glm::vec3& center, f32 radius, const glm::vec3& color);
    // void addCircle(const glm::vec3& center, const glm::vec3& normal, f32 radius, const glm::vec3& color);
    // void addCylinder(const glm::vec3& start, const glm::vec3& end, f32 radius, const glm::vec3& color);

private:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    struct BoxInstance {
        glm::mat4 transform;
        glm::vec3 color;
    };

    GLShader m_lineShader;
    GLShader m_objectShader;

    std::vector<BoxInstance> m_boxes;
    std::vector<Vertex> m_vertices;

    u32 m_lineVao = 0;
    u32 m_lineVbo = 0;
    u32 m_boxVao = 0;
    u32 m_boxVbo = 0;
    u32 m_boxIbo = 0;
    u32 m_boxInstanceVbo = 0;

    u32 m_viewLocation = 0;
    u32 m_projLocation = 0;

    // struct Sphere {
    //     glm::vec3 center;
    //     f32 radius;
    //     glm::vec3 color;
    // };

    // struct Circle {
    //     glm::vec3 center;
    //     glm::vec3 normal;
    //     f32 radius;
    //     glm::vec3 color;
    // };

    // struct Cylinder {
    //     glm::vec3 start;
    //     glm::vec3 end;
    //     f32 radius;
    //     glm::vec3 color;
    // };
};
