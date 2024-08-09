#pragma once

#include "types.h"
#include "spl/spl_particle.h"

#include <vector>


struct ParticleInstance {
    glm::mat4 transform;

};

class ParticleRenderer {
public:
    explicit ParticleRenderer(u32 maxInstances);

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

    glm::mat4 m_view;
    glm::mat4 m_proj;
    s32 m_viewLocation;
    s32 m_projLocation;

    std::vector<glm::mat4> m_transforms;
};
