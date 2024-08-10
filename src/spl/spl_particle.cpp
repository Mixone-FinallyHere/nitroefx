#include "spl_particle.h"

#include <glm/ext/matrix_transform.hpp>

#include "editor/particle_renderer.h"


void SPLParticle::render(ParticleRenderer* renderer, const glm::vec3& cameraPos, f32 s, f32 t) const {
    const auto transform = glm::translate(glm::mat4(1.0f), getWorldPosition());
    renderer->submit(texture, { 
        .color = glm::vec4(color, visibility.baseAlpha * visibility.animAlpha),
        .transform = transform,
        .texCoords = {
            { 0.0f, 0.0f }, // bottom left
            { s,    0.0f }, // bottom right
            { s,    t    }, // top right
            { 0.0f, t    }  // top left
        }
    });
}

glm::vec3 SPLParticle::getWorldPosition() const {
    return emitterPos + position;
}
