#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "spl_behavior.h"
#include "types.h"
#include "fx.h"

struct SPLFileHeader {
    u32 magic;
    u32 version;
    u16 resCount;
    u16 texCount;
    u32 reserved0;
    u32 resSize;
    u32 texSize;
    u32 texOffset;
    u32 reserved1;
};

union SPLResourceFlags {
    u32 all;
    struct {
        u32 emissionType : 4; // Maps to SPLEmissionType
        u32 drawType : 2;
        u32 circleAxis : 2; // Maps to SPLCircleAxis
        u32 hasScaleAnim : 1;
        u32 hasColorAnim : 1;
        u32 hasAlphaAnim : 1;
        u32 hasTexAnim : 1;
        u32 hasRotation : 1;
        u32 randomInitAngle : 1;
        // Whether the emitter manages itself or not.
        // If set, the emitter will automatically terminate when it reaches the end of its life
        // and all of its particles have died
        u32 selfMaintaining : 1;
        u32 followEmitter : 1;
        u32 hasChildResource : 1;
        u32 polygonRotAxis : 2; // The axis to rotate the polygon around when using the 'polygon' draw types
        u32 polygonReferencePlane : 1;
        u32 randomizeLoopedAnim : 1;
        u32 drawChildrenFirst : 1; // If set, child particles will be rendered before parent particles
        u32 hideParent : 1; // If set, only child particles will be rendered
        u32 useViewSpace : 1; // If set, the rendering calculations will be done in view space
        u32 hasGravityBehavior : 1;
        u32 hasRandomBehavior : 1;
        u32 hasMagnetBehavior : 1;
        u32 hasSpinBehavior : 1;
        u32 hasCollisionPlaneBehavior : 1;
        u32 hasConvergenceBehavior : 1;
        u32 hasFixedPolygonID : 1;
        u32 childHasFixedPolygonID : 1;
    };
};

union SPLChildResourceFlags {
    u16 all;
    struct {
        u16 usesBehaviors : 1;
        u16 hasScaleAnim : 1;
        u16 hasAlphaAnim : 1;
        u16 rotationType : 2;
        u16 followEmitter : 1;
        u16 useChildColor : 1;
        u16 drawType : 2;
        u16 polygonRotAxis : 2;
        u16 polygonReferencePlane : 1;
        u16 : 4;
    };
};

struct SPLCurveInOut {
    u16 in : 8;
    u16 out : 8;
};

struct SPLCurveInPeakOut {
    u32 in : 8;
    u32 peak : 8;
    u32 out : 8;
    u32 : 8;
};

template<class TFloat32, class TFloat16, class TVecF32, class TVecF16, class TColor>
struct SPLResourceHeaderTemplate {
    SPLResourceFlags flags;
    TVecF32 emitterBasePos;
    TFloat32 emissionCount; // Number of particles to emit per emission interval
    TFloat32 radius; // Used for circle, sphere, and cylinder emissions
    TFloat32 length; // Used for cylinder emission
    TVecF16 axis;
    TColor color;
    TFloat32 initVelPosAmplifier;
    TFloat32 initVelAxisAmplifier;
    TFloat32 baseScale;
    TFloat16 aspectRatio;
    u16 startDelay; // Delay, in frames, before the emitter starts emitting particles
    s16 minRotation;
    s16 maxRotation;
    u16 initAngle;
    u16 reserved;
    u16 emitterLifeTime;
    u16 particleLifeTime;

    // All of these values are mapped to the range [0, 1]
    // They are used to attenuate the particle's properties at initialization,
    // acting as a sort of randomization factor which scales down the initial values
    struct {
        u32 baseScale : 8; // Damping factor for the base scale of the particles (0 = no damping)
        u32 lifeTime : 8;
        u32 initVel : 8; // Attenuation factor for the initial velocity of the particles (0 = no attenuation)
        u32 : 8;
    } randomAttenuation;

    struct {
        u32 emissionInterval : 8;
        u32 baseAlpha : 8;
        u32 airResistance : 8;
        u32 textureIndex : 8;
        u32 loopFrames : 8;
        u32 dbbScale : 16;
        u32 textureTileCountS : 2; // Number of times to tile the texture in the S direction
        u32 textureTileCountT : 2; // Number of times to tile the texture in the T direction
        u32 scaleAnimDir : 3; // Maps to SPLScaleAnimDir
        u32 dpolFaceEmitter : 1; // If set, the polygon will face the emitter
        u32 flipTextureS : 1;
        u32 flipTextureT : 1;
        u32 : 30;
    } misc;
    TFloat16 polygonX;
    TFloat16 polygonY;
    struct {
        u32 flags : 8;
        u32 : 24;
    } userData;
};

template<class TFloat16>
struct SPLScaleAnimTemplate {
    TFloat16 start;
    TFloat16 mid;
    TFloat16 end;
    SPLCurveInOut curve;
    struct {
        u16 loop : 1;
        u16 : 15;
    } flags;
    u16 padding;
};

template<class TColor>
struct SPLColorAnimTemplate {
    TColor start;
    TColor end;
    SPLCurveInPeakOut curve;
    struct {
        u16 randomStartColor : 1;
        u16 loop : 1;
        u16 interpolate : 1;
        u16 : 13;
    } flags;
    u16 padding;
};

struct SPLAlphaAnim {
    union {
        u16 all;
        struct {
            u16 start : 5;
            u16 mid : 5;
            u16 end : 5;
            u16 : 1;
        };
    } alpha;
    struct {
        u16 randomRange : 8;
        u16 loop : 1;
        u16 : 7;
    } flags;
    SPLCurveInOut curve;
    u16 padding;
};

struct SPLTexAnim {
    u8 textures[8];
    struct {
        u32 frameCount : 8;
        u32 step : 8; // Number of frames between each texture frame
        u32 randomizeInit : 1; // Randomize the initial texture frame
        u32 loop : 1;
        u32 : 14;
    } param;
};

template<class TFloat16, class TColor>
struct SPLChildResourceTemplate {
    SPLChildResourceFlags flags;
    TFloat16 randomInitVelMag; // Randomization factor for the initial velocity magnitude (0 = no randomization)
    TFloat16 endScale; // For scaling animations
    u16 lifeTime;
    u8 velocityRatio; // Ratio of the parent particle's velocity to inherit (255 = 100%)
    u8 scaleRatio; // Ratio of the parent particle's scale to inherit (255 = 100%)
    TColor color;
    struct {
        u32 emissionCount : 8; // Number of particles to emit per emission interval
        u32 emissionDelay : 8; // Delay, as a fraction of the particle's lifetime, before the particle starts emitting
        u32 emissionInterval : 8;
        u32 texture : 8;
        u32 textureTileCountS : 2;
        u32 textureTileCountT : 2;
        u32 flipTextureS : 1;
        u32 flipTextureT : 1;
        u32 dpolFaceEmitter : 1; // If set, the polygon will face the emitter
        u32 : 25;
    } misc;
};

union SPLTextureParam {
    u32 all;
    struct {
        u32 format : 4; // Maps to GXTexFmt
        u32 s : 4; // Maps to GXTexSizeS
        u32 t : 4; // Maps to GXTexSizeT
        u32 repeat : 2; // Maps to GXTexRepeat
        u32 flip : 2; // Maps to GXTexFlip
        u32 palColor0 : 1; // Maps to GXTexPlttColor0
        u32 useSharedTexture : 1;
        u32 sharedTexID : 8;
        u32 : 6;
    };
};

struct SPLTextureResource {
    u32 id;
    SPLTextureParam param;
    u32 textureSize; // size of the texture data
    u32 paletteOffset; // offset to the palette data from the start of the header
    u32 paletteSize; // size of the palette data
    u32 unused0;
    u32 unused1;
    u32 resourceSize; // total size of the resource (header + data)
};

struct SPLTexture {
    const SPLTextureResource* resource;
    SPLTextureParam param;
    u16 width;
    u16 height;
    std::vector<u8> textureData;
    std::vector<u8> paletteData;
};

using SPLResourceHeader = SPLResourceHeaderTemplate<f32, f32, glm::vec3, glm::vec3, glm::vec3>;
using SPLResourceHeaderNative = SPLResourceHeaderTemplate<fx32, fx16, VecFx32, VecFx16, GXRgb>;

using SPLScaleAnim = SPLScaleAnimTemplate<f32>;
using SPLScaleAnimNative = SPLScaleAnimTemplate<fx16>;

using SPLColorAnim = SPLColorAnimTemplate<glm::vec3>;
using SPLColorAnimNative = SPLColorAnimTemplate<GXRgb>;

using SPLAlphaAnimNative = SPLAlphaAnim;

using SPLTexAnimNative = SPLTexAnim;

using SPLChildResource = SPLChildResourceTemplate<f32, glm::vec3>;
using SPLChildResourceNative = SPLChildResourceTemplate<fx16, GXRgb>;

struct SPLResource {
    SPLResourceHeader header;
    std::optional<SPLScaleAnim> scaleAnim;
    std::optional<SPLColorAnim> colorAnim;
    std::optional<SPLAlphaAnim> alphaAnim;
    std::optional<SPLTexAnim> texAnim;
    std::optional<SPLChildResource> childResource;
    std::vector<std::shared_ptr<SPLBehavior>> behaviors;
};
