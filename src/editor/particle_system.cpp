#include "particle_system.h"


SPLParticle* ParticleSystem::allocateParticle() {
    if (m_availableParticles.empty()) {
        return nullptr;
    }

    const auto particle = m_availableParticles.front();
    m_availableParticles.pop();

    return particle;
}

void ParticleSystem::freeParticle(SPLParticle* particle) {
    m_availableParticles.push(particle);
}
