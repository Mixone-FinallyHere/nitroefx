#pragma once

#include "types.h"
#include "spl/spl_particle.h"

#include <span>
#include <unordered_map>
#include <vector>

#include "gfx/gl_shader.h"
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

    const glm::mat4& getView() const { return m_view; }

    void setTextures(std::span<const SPLTexture> textures);
    void setMaxInstances(u32 maxInstances);

private:
    u32 m_maxInstances;
    u32 m_vao;
    u32 m_vbo;
    u32 m_ibo;
    u32 m_transformVbo;
    GLShader m_shader;

    std::span<const SPLTexture> m_textures;
    glm::mat4 m_view;
    glm::mat4 m_proj;
    s32 m_viewLocation;
    s32 m_projLocation;
    s32 m_textureLocation;
    bool m_isRendering = false;

    size_t m_particleCount = 0;
    std::vector<std::vector<ParticleInstance>> m_particles;
};
