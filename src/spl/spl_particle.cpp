#include "spl_particle.h"
#include "editor/camera.h"
#include "editor/particle_renderer.h"
#include "spl_emitter.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>



void SPLParticle::render(ParticleRenderer* renderer, const CameraParams& params, f32 s, f32 t) const {
    switch (emitter->getResource()->header.flags.drawType) {
    case SPLDrawType::Billboard:
        renderBillboard(renderer, params, s, t);
        break;
    case SPLDrawType::DirectionalBillboard:
        renderDirectionalBillboard(renderer, params, s, t);
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
    return emitterPos + position;
}

void SPLParticle::renderBillboard(ParticleRenderer* renderer, const CameraParams& params, f32 s, f32 t) const {
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

    const auto particlePos = getWorldPosition();
    const auto viewAxis = glm::normalize(params.pos - particlePos);

    auto orientation = glm::mat4(1);
    orientation[0] = glm::vec4(params.right, 0);
    orientation[1] = glm::vec4(params.up, 0);
    orientation[2] = glm::vec4(viewAxis, 0);

    const auto transform = glm::translate(glm::mat4(1), particlePos)
        * orientation
        * glm::rotate(glm::mat4(1), rotation, { 0, 0, 1 })
        * glm::scale(glm::mat4(1), scale);

    renderer->submit(texture, {
        .color = { color, visibility.baseAlpha * visibility.animAlpha },
        .transform = transform,
        .texCoords = {
            { 0, t },
            { s, t },
            { s, 0 },
            { 0, 0 }
        }
    });
}

void SPLParticle::renderDirectionalBillboard(ParticleRenderer* renderer, const CameraParams& params, f32 s, f32 t) const {
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

    glm::vec3 dir = glm::cross(velocity, params.forward);
    if (glm::length2(dir) < 0.0001f) {
        return;
    }
    
    dir = glm::normalize(dir);
    const auto velDir = glm::normalize(velocity);

    f32 dot = glm::dot(velDir, -params.forward);
    if (dot < 0.0f) { // Particle is behind the camera
        dot = -dot;
    }

    scale.y *= (1.0f - dot) * resource->header.misc.dbbScale + 1.0f;
    const auto pos = glm::vec4(getWorldPosition(), 1) * params.view;
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
