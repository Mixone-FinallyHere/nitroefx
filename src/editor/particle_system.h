#pragma once

#include "spl/spl_particle.h"
#include "spl/spl_emitter.h"

#include <queue>


class ParticleSystem {

public:
    SPLParticle* allocateParticle();
    void freeParticle(SPLParticle* particle);

private:
    std::queue<SPLParticle*> m_availableParticles;

    SPLParticle* m_particles;
};
