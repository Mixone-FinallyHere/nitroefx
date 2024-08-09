#include "spl_particle.h"

#include <glm/ext/matrix_transform.hpp>

#include "editor/particle_renderer.h"


void SPLParticle::render(ParticleRenderer* renderer, const glm::vec3& cameraPos) const {
    const auto transform = glm::translate(glm::mat4(1.0f), getWorldPosition());
    renderer->submit({ 
        glm::vec4(color, visibility.baseAlpha * visibility.animAlpha),
        transform
    });
}

glm::vec3 SPLParticle::getWorldPosition() const {
    return emitterPos + position;
}
