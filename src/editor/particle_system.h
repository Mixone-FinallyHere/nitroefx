#pragma once

#include "spl/spl_particle.h"
#include "spl/spl_emitter.h"
#include "particle_renderer.h"

#include <queue>
#include <vector>

class ParticleSystem {
public:
    explicit ParticleSystem(u32 maxParticles);
    ~ParticleSystem();

    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);

    std::weak_ptr<SPLEmitter> addEmitter(const SPLResource& resource, bool looping = false);

    SPLParticle* allocateParticle();
    void freeParticle(SPLParticle* particle);

    ParticleRenderer* getRenderer() { return &m_renderer; }

private:
    ParticleRenderer m_renderer;
    std::queue<SPLParticle*> m_availableParticles;
    std::vector<std::shared_ptr<SPLEmitter>> m_emitters;
    bool m_cycle =false;

    SPLParticle* m_particles;
};
