#include "particle_system.h"


ParticleSystem::ParticleSystem(u32 maxParticles) {
    m_particles = new SPLParticle[maxParticles];

    for (u32 i = 0; i < maxParticles; i++) {
        m_availableParticles.push(&m_particles[i]);
    }
}

ParticleSystem::~ParticleSystem() {
    delete[] m_particles;
}

void ParticleSystem::update(float deltaTime) {
    for (auto it = m_emitters.begin(); it != m_emitters.end();) {
        auto& emitter = *it;
        const auto& header = emitter.m_resource->header;

        if (!emitter.m_state.started && emitter.m_age >= header.startDelay) {
            emitter.m_state.started = true;
            emitter.m_age = 0;
        }

        if (!emitter.m_state.paused) {
            if (emitter.m_updateCycle == 0 || (u8)m_cycle == emitter.m_updateCycle - 1) {
                emitter.update(deltaTime);
            }
        }

        if (emitter.shouldTerminate()) {
            it = m_emitters.erase(it);
        } else {
            ++it;
        }
    }

    m_cycle = !m_cycle;
}

void ParticleSystem::addEmitter(const SPLResource& resource) {
    m_emitters.emplace_back(&resource, this);
}

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
