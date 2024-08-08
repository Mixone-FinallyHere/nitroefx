#pragma once

#include "spl/spl_particle.h"
#include "spl/spl_emitter.h"

#include <list>
#include <queue>

class ParticleSystem {
public:
    ParticleSystem(u32 maxParticles);
    ~ParticleSystem();

    void update(float deltaTime);

    void addEmitter(const SPLResource& resource);

    SPLParticle* allocateParticle();
    void freeParticle(SPLParticle* particle);

private:
    std::queue<SPLParticle*> m_availableParticles;
    std::list<SPLEmitter> m_emitters;
    bool m_cycle =false;

    SPLParticle* m_particles;
};
