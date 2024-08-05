#pragma once

#include <functional>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "types.h"
#include "fx.h"

struct SPLParticle;
struct SPLEmitter;

enum class SPLSpinAxis : u16 {
    X = 0,
    Y,
    Z
};

enum class SPLCollisionType : u16 {
    Kill = 0,
    Bounce,
};

struct SPLBehavior {
    virtual void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) = 0;
};

// Applies a gravity behavior to particles
struct SPLGravityBehaviorNative {
    VecFx16 magnitude;
    u16 padding;
};

// Applies a random movement behavior to particles
struct SPLRandomBehaviorNative {
    VecFx16 magnitude;
    u16 applyInterval; // The interval, in frames, at which the random behavior is applied
};

// Applies a magnetic force to particles
struct SPLMagnetBehaviorNative {
    VecFx32 target;
    fx16 force;
    u16 padding;
};

// Applies a rotational force to particles
struct SPLSpinBehaviorNative {
    u16 angle; // The angle of rotation, in "index" units, where 0x10000 is a full rotation. Applied every frame
    u16 axis; // The axis of rotation
};

// Applies a collision behavior to particles
struct SPLCollisionPlaneBehaviorNative {
    fx32 y; // The Y position of the collision plane
    fx16 elasticity; // The elasticity of the collision, 1.0 being perfectly elastic and 0.0 being perfectly inelastic

    struct {
        u16 collisionType : 2;
        u16 : 14;
    } flags;
};

// Applies a convergence behavior to particles
// Similar to SPLMagnetBehavior, but it acts directly on the particle's position instead of its acceleration
struct SPLConvergenceBehaviorNative {
    VecFx32 target;
    fx16 force;
    u16 padding;
};


struct SPLGravityBehavior : SPLBehavior {
    glm::vec3 magnitude;

    explicit SPLGravityBehavior(const SPLGravityBehaviorNative& native) 
        : magnitude(native.magnitude.toVec3()) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLGravityBehavior::apply not implemented");
    }
};

struct SPLRandomBehavior : SPLBehavior {
    glm::vec3 magnitude;
    u16 applyInterval;

    explicit SPLRandomBehavior(const SPLRandomBehaviorNative& native) 
        : magnitude(native.magnitude.toVec3()), applyInterval(native.applyInterval) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLRandomBehavior::apply not implemented");
    }
};

struct SPLMagnetBehavior : SPLBehavior {
    glm::vec3 target;
    f32 force;

    explicit SPLMagnetBehavior(const SPLMagnetBehaviorNative& native) 
        : target(native.target.toVec3()), force(FX_FX16_TO_F32(native.force)) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLMagnetBehavior::apply not implemented");
    }
};

struct SPLSpinBehavior : SPLBehavior {
    u16 angle;
    SPLSpinAxis axis;

    explicit SPLSpinBehavior(const SPLSpinBehaviorNative& native) 
        : angle(native.angle), axis(static_cast<SPLSpinAxis>(native.axis)) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLSpinBehavior::apply not implemented");
    }
};

struct SPLCollisionPlaneBehavior : SPLBehavior {
    f32 y;
    f32 elasticity;
    SPLCollisionType collisionType;

    explicit SPLCollisionPlaneBehavior(const SPLCollisionPlaneBehaviorNative& native) 
        : y(FX_FX32_TO_F32(native.y)), elasticity(FX_FX16_TO_F32(native.elasticity)), 
        collisionType(static_cast<SPLCollisionType>(native.flags.collisionType)) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLCollisionPlaneBehavior::apply not implemented");
    }
};

struct SPLConvergenceBehavior : SPLBehavior {
    glm::vec3 target;
    f32 force;

    explicit SPLConvergenceBehavior(const SPLConvergenceBehaviorNative& native) 
        : target(native.target.toVec3()), force(FX_FX16_TO_F32(native.force)) {}

    void apply(SPLParticle& particle, glm::vec3& acceleration, SPLEmitter& emitter) override {
        spdlog::warn("SPLConvergenceBehavior::apply not implemented");
    }
};


