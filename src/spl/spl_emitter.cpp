#include "spl_emitter.h"

SPLEmitter::SPLEmitter(SPLResource* resource, const glm::vec3& pos) {
    m_resource = resource;
    m_state = {};

    m_position = pos + resource->header.emitterBasePos;
    m_particleInitVelocity = {};
    m_velocity = {};

    m_age = 0;

    m_axis = resource->header.axis;
    m_initAngle = resource->header.initAngle;
    m_emissionCount = resource->header.emissionCount;
    m_radius = resource->header.radius;
    m_length = resource->header.length;
    m_initVelPositionAmplifier = resource->header.initVelPosAmplifier;
    m_initVelAxisAmplifier = resource->header.initVelAxisAmplifier;
    m_baseScale = resource->header.baseScale;
    m_particleLifeTime = resource->header.particleLifeTime;

    m_color = resource->header.color;
    m_emissionInterval = resource->header.misc.emissionInterval;
    m_baseAlpha = resource->header.misc.baseAlpha;
    m_updateCycle = 0;
    m_collisionPlaneHeight = std::numeric_limits<f32>::min();

    // Textures can only be tiled in powers of 2
    m_texCoords = {
        glm::pow(2.0f, resource->header.misc.textureTileCountS),
        glm::pow(2.0f, resource->header.misc.textureTileCountT)
    };

    if (resource->header.misc.flipTextureS) {
        m_texCoords.x = -m_texCoords.x;
    }

    if (resource->header.misc.flipTextureT) {
        m_texCoords.y = -m_texCoords.y;
    }

    if (resource->header.flags.hasChildResource && resource->childResource) {
        m_childTexCoords = {
            resource->childResource->misc.textureTileCountS,
            resource->childResource->misc.textureTileCountT
        };

        if (resource->childResource->misc.flipTextureS) {
            m_childTexCoords.x = -m_childTexCoords.x;
        }

        if (resource->childResource->misc.flipTextureT) {
            m_childTexCoords.y = -m_childTexCoords.y;
        }
    }

    m_crossAxis1 = {};
    m_crossAxis2 = {};
}

SPLEmitter::~SPLEmitter() = default;

void SPLEmitter::update() {
}

void SPLEmitter::render() {
}
