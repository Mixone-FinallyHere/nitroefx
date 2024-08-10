#pragma once

#include "types.h"
#include "spl/spl_particle.h"

#include <span>
#include <unordered_map>
#include <vector>

#include "spl/spl_resource.h"


struct ParticleInstance {
    glm::vec4 color;
    glm::mat4 transform;
    glm::vec2 texCoords[4];
};

class ParticleRenderer {
public:
    explicit ParticleRenderer(u32 maxInstances, std::span<const SPLTexture> textures);

    void begin(const glm::mat4& view, const glm::mat4& proj);
    void end();

    void submit(u32 texture, const ParticleInstance& instance);

private:
    u32 m_maxInstances;
    u32 m_vao;
    u32 m_vbo;
    u32 m_ibo;
    u32 m_shader;
    u32 m_transformVbo;

    std::span<const SPLTexture> m_textures;
    glm::mat4 m_view;
    glm::mat4 m_proj;
    s32 m_viewLocation;
    s32 m_projLocation;
    s32 m_textureLocation;

    std::vector<std::vector<ParticleInstance>> m_particles;
};
