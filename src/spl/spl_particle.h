#pragma once

#include <glm/glm.hpp>

#include "spl_list.h"
#include "types.h"


struct SPLParticle : SPLNode<SPLParticle> {
    glm::vec3 position; // position of the particle, relative to the emitter
    glm::vec3 velocity;
    u16 rotation;
    s16 angularVelocity;
    u16 lifeTime;
    u16 age;

    // These two values are essentially 1.0f / lifeTime (or 1.0f / loopTime), represented as an integer
    // They are used to map between age/lifeTime and a [0, 255] range
    // Mainly just to lower the amount of divisions needed in the update functions
    u16 loopTimeFactor;
    u16 lifeTimeFactor;

    u8 texture; // Index of the current texture in the resource

    // A value between 0 and 255 that is added to the life rate of the particle.
    // This is used only for looping particles, so particles spawned at the same time
    // don't have aren't all in sync (animation-wise)
    u8 lifeRateOffset;

    struct {
        u16 baseAlpha : 5;
        u16 animAlpha : 5;
        u16 currentPolygonID : 6;
    } visibility;
    f32 baseScale;
    f32 animScale;
    glm::vec3 color;
    glm::vec3 emitterPos;
};
