#pragma once

#include "types.h"
#include "spl/spl_particle.h"

#include <vector>


struct ParticleInstance {
    glm::vec4 color;
    glm::mat4 transform;
    glm::vec2 texCoords[4];
    f32 texIndex;
};

class ParticleRenderer {
public:
    explicit ParticleRenderer(u32 maxInstances, u32 textureArray);

    void begin(const glm::mat4& view, const glm::mat4& proj);
    void end();

    void submit(const ParticleInstance& instance);

private:
    u32 m_maxInstances;
    u32 m_vao;
    u32 m_vbo;
    u32 m_ibo;
    u32 m_shader;
    u32 m_transformVbo;
    u32 m_textureArray;

    glm::mat4 m_view;
    glm::mat4 m_proj;
    s32 m_viewLocation;
    s32 m_projLocation;
    s32 m_textureLocation;

    std::vector<ParticleInstance> m_transforms;

};
