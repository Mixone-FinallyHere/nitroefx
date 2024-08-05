#pragma once

#include "spl_resource.h"
#include "spl_particle.h"
#include "spl_list.h"
#include "types.h"


struct SPLEmitterState {
    union {
        u32 all;
        struct
        {
            u32 terminate : 1;
            u32 emissionPaused : 1;
            u32 paused : 1;
            u32 renderingDisabled : 1;
            u32 started : 1;
            u32 : 27;
        };
    };
};

struct SPLEmitter : SPLNode<SPLEmitter> {
    SPLList<SPLParticle> particles;
    SPLList<SPLParticle> childParticles;
    SPLResource *resource;
    SPLEmitterState state;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 particleInitVelocity;
    u16 age;
    f32 emissionCountFractional; // fractional part of the number of particles to emit (doesn't seem to be used)
    glm::vec3 axis;
    u16 initAngle;
    f32 emissionCount;
    f32 radius;
    f32 length;
    f32 initVelPositionAmplifier; // amplifies the initial velocity of the particles based on their position
    f32 initVelAxisAmplifier; // amplifies the initial velocity of the particles based on the emitter's axis
    f32 baseScale; // base scale of the particles
    u16 particleLifeTime;
    glm::vec3 color;
    f32 collisionPlaneHeight;
    f32 textureS;
    f32 textureT;
    f32 childTextureS;
    f32 childTextureT;
    struct {
        u32 emissionInterval : 8; // number of frames between particle emissions
        u32 baseAlpha : 8;
        u32 updateCycle : 3; // 0 = every frame, 1 = cycle A, 2 = cycle B, cycles A and B alternate
        u32 reserved : 13;
    } misc;
    glm::vec3 crossAxis1;
    glm::vec3 crossAxis2;
};
