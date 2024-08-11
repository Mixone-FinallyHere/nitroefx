#include "spl_particle.h"
#include "editor/particle_renderer.h"
#include "spl_emitter.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>



void SPLParticle::render(ParticleRenderer* renderer, const glm::vec3& cameraPos, f32 s, f32 t) const {
    switch (emitter->getResource()->header.flags.drawType) {
    case SPLDrawType::Billboard:
        renderBillboard(renderer, cameraPos, s, t);
        break;
    case SPLDrawType::DirectionalBillboard:
        renderDirectionalBillboard(renderer, cameraPos, s, t);
        break;
    case SPLDrawType::Polygon:
        break;
    case SPLDrawType::DirectionalPolygon:
        break;
    case SPLDrawType::DirectionalPolygonCenter:
        break;
    }
}

glm::vec3 SPLParticle::getWorldPosition() const {
    return emitterPos + position + emitter->getResource()->header.emitterBasePos;
}

void SPLParticle::renderBillboard(ParticleRenderer* renderer, const glm::vec3& cameraPos, f32 s, f32 t) const {
    const SPLResource* resource = emitter->getResource();
    glm::vec3 scale = { baseScale * resource->header.aspectRatio, baseScale, 1 };

    switch (resource->header.misc.scaleAnimDir) {
    case SPLScaleAnimDir::XY:
        scale.x *= animScale;
        scale.y *= animScale;
        break;
    case SPLScaleAnimDir::X:
        scale.x *= animScale;
        break;
    case SPLScaleAnimDir::Y:
        scale.y *= animScale;
        break;
    }

    const auto transform = glm::translate(glm::mat4(1), getWorldPosition())
        * glm::rotate(glm::mat4(1), rotation, { 0, 0, 1 })
        * glm::scale(glm::mat4(1), scale);

    renderer->submit(texture, {
        .color = { color, visibility.baseAlpha * visibility.animAlpha },
        .transform = transform,
        .texCoords = {
            { 0, 0 },
            { s, 0 },
            { s, t },
            { 0, t }
        }
    });
}

void SPLParticle::renderDirectionalBillboard(ParticleRenderer* renderer, const glm::vec3& cameraPos, f32 s, f32 t) const {
    const SPLResource* resource = emitter->getResource();
    glm::vec3 scale = { baseScale * resource->header.aspectRatio, baseScale, 1 };

    switch (resource->header.misc.scaleAnimDir) {
    case SPLScaleAnimDir::XY:
        scale.x *= animScale;
        scale.y *= animScale;
        break;
    case SPLScaleAnimDir::X:
        scale.x *= animScale;
        break;
    case SPLScaleAnimDir::Y:
        scale.y *= animScale;
        break;
    }

    const auto& view = renderer->getView();
    const auto cameraDir = glm::vec3(-view[0][2], -view[1][2], -view[2][2]);

    glm::vec3 dir = glm::cross(velocity, cameraDir);
    if (glm::length2(dir) < 0.0001f) {
        return;
    }
    
    dir = glm::normalize(dir);
    const auto velDir = glm::normalize(velocity);

    const f32 dot = glm::dot(velDir, -cameraDir);
    if (dot < 0.0f) { // Particle is behind the camera
        return;
    }

    scale.y *= (1.0f - dot) * resource->header.misc.dbbScale + 1.0f;
    const auto pos = getWorldPosition();
    const auto transform = glm::mat4(
        dir.x * scale.x, dir.y * scale.x, 0, 0,
        -dir.y * scale.y, dir.x * scale.y, 0, 0,
        0, 0, 1, 0,
        pos.x, pos.y, pos.z, 1
    );

    renderer->submit(texture, {
        .color = { color, visibility.baseAlpha * visibility.animAlpha },
        .transform = transform,
        .texCoords = {
            { 0, 0 },
            { s, 0 },
            { s, t },
            { 0, t }
        }
    });
}
