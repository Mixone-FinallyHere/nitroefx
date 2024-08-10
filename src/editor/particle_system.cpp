#include "particle_system.h"


ParticleSystem::ParticleSystem(u32 maxParticles, std::span<const SPLTexture> textures)
    : m_renderer(maxParticles, textures) {
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
        const auto& emitter = *it;
        const auto& header = emitter->m_resource->header;

        if (!emitter->m_state.started && emitter->m_age >= header.startDelay) {
            emitter->m_state.started = true;
            emitter->m_age = 0;
        }

        if (!emitter->m_state.paused) {
            if (emitter->m_updateCycle == 0 || (u8)m_cycle == emitter->m_updateCycle - 1) {
                emitter->update(deltaTime);
            }
        }

        if (emitter->shouldTerminate()) {
            it = m_emitters.erase(it);
        } else {
            ++it;
        }
    }

    m_cycle = !m_cycle;
}

void ParticleSystem::render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    m_renderer.begin(view, proj);

    for (auto& emitter : m_emitters) {
        if (!emitter->m_state.renderingDisabled) {
            emitter->render(cameraPos);
        }
    }

    m_renderer.end();
}

std::weak_ptr<SPLEmitter> ParticleSystem::addEmitter(const SPLResource& resource, bool looping) {
    m_emitters.emplace_back(std::make_shared<SPLEmitter>(&resource, this, looping));
    return m_emitters.back();
}

void ParticleSystem::killEmitter(const std::weak_ptr<SPLEmitter>& emitter) const {
    if (const auto shared = emitter.lock()) {
        shared->m_state.terminate = true;
    }
}

void ParticleSystem::killAllEmitters() const {
    for (const auto& emitter : m_emitters) {
        emitter->m_state.terminate = true;
    }
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
