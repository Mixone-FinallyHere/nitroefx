#include "spl_archive.h"

#include <spdlog/spdlog.h>


SPLArchive::SPLArchive() : m_header(), m_loaded(false) {
}

SPLArchive::~SPLArchive() {
    if (m_loaded) {
        unload();
    }
}


void SPLArchive::load(std::string_view filename) {
    if (m_loaded) {
        unload();
    }

    FILE* file = fopen(filename.data(), "rb");
    if (!file) {
        spdlog::error("Failed to open file: {}", filename);
        return;
    }

    fread(&m_header, sizeof(SPLFileHeader), 1, file);

    m_resources.resize(m_header.resCount);
    
    for (size_t i = 0; i < m_header.resCount; i++) {
        SPLResource& res = m_resources[i];

        SPLResourceHeaderNative header;
        fread(&header, sizeof(SPLResourceHeaderNative), 1, file);

        res.header = fromNative(header);
        const auto& flags = res.header.flags;

        // Animations
        if (flags.hasScaleAnim) {
            SPLScaleAnimNative scaleAnim;
            fread(&scaleAnim, sizeof(SPLScaleAnimNative), 1, file);
            res.scaleAnim = fromNative(scaleAnim);
        }

        if (flags.hasColorAnim) {
            SPLColorAnimNative colorAnim;
            fread(&colorAnim, sizeof(SPLColorAnimNative), 1, file);
            res.colorAnim = fromNative(colorAnim);
        }

        if (flags.hasAlphaAnim) {
            SPLAlphaAnimNative alphaAnim;
            fread(&alphaAnim, sizeof(SPLAlphaAnim), 1, file);
            res.alphaAnim = fromNative(alphaAnim);
        }

        if (flags.hasTexAnim) {
            SPLTexAnimNative texAnim;
            fread(&texAnim, sizeof(SPLTexAnimNative), 1, file);
            res.texAnim = fromNative(texAnim);
        }

        if (flags.hasChildResource) {
            SPLChildResourceNative childResource;
            fread(&childResource, sizeof(SPLChildResourceNative), 1, file);
            res.childResource = fromNative(childResource);
        }

        // Behaviors
        if (flags.hasGravityBehavior) {
            SPLGravityBehaviorNative gravityBehavior;
            fread(&gravityBehavior, sizeof(SPLGravityBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(gravityBehavior));
        }

        if (flags.hasRandomBehavior) {
            SPLRandomBehaviorNative randomBehavior;
            fread(&randomBehavior, sizeof(SPLRandomBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(randomBehavior));
        }

        if (flags.hasMagnetBehavior) {
            SPLMagnetBehaviorNative magnetBehavior;
            fread(&magnetBehavior, sizeof(SPLMagnetBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(magnetBehavior));
        }

        if (flags.hasSpinBehavior) {
            SPLSpinBehaviorNative spinBehavior;
            fread(&spinBehavior, sizeof(SPLSpinBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(spinBehavior));
        }

        if (flags.hasCollisionPlaneBehavior) {
            SPLCollisionPlaneBehaviorNative collisionPlaneBehavior;
            fread(&collisionPlaneBehavior, sizeof(SPLCollisionPlaneBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(collisionPlaneBehavior));
        }

        if (flags.hasConvergenceBehavior) {
            SPLConvergenceBehaviorNative convergenceBehavior;
            fread(&convergenceBehavior, sizeof(SPLConvergenceBehaviorNative), 1, file);
            res.behaviors.push_back(fromNative(convergenceBehavior));
        }
    }

    m_textures.resize(m_header.texCount);

    for (size_t i = 0; i < m_header.texCount; i++) {
        SPLTexture& tex = m_textures[i];
        SPLTextureResource texRes;
        size_t offset = ftell(file);
        fread(&texRes, sizeof(SPLTextureResource), 1, file);

        tex.textureData.resize(texRes.textureSize);
        tex.paletteData.resize(texRes.paletteSize);

        fread(tex.textureData.data(), 1, texRes.textureSize, file);
        fseek(file, offset + texRes.paletteOffset, SEEK_SET);
        fread(tex.paletteData.data(), 1, texRes.paletteSize, file);

        tex.resource = nullptr;
        tex.param = texRes.param;
        tex.width = 1 << (texRes.param.s + 3);
        tex.height = 1 << (texRes.param.t + 3);
    }

    fclose(file);
    m_loaded = true;
}

void SPLArchive::unload() {
    m_resources.clear();
    m_textures.clear();
    m_loaded = false;
}

SPLResourceHeader SPLArchive::fromNative(const SPLResourceHeaderNative &native) {
    return SPLResourceHeader {
        .flags = native.flags,
        .emitterBasePos = native.emitterBasePos.toVec3(),
        .emissionCount = FX_FX32_TO_F32(native.emissionCount),
        .radius = FX_FX32_TO_F32(native.radius),
        .length = FX_FX32_TO_F32(native.length),
        .axis = native.axis.toVec3(),
        .color = native.color.toVec3(),
        .initVelPosAmplifier = FX_FX32_TO_F32(native.initVelPosAmplifier),
        .initVelAxisAmplifier = FX_FX32_TO_F32(native.initVelAxisAmplifier),
        .baseScale = FX_FX32_TO_F32(native.baseScale),
        .aspectRatio = FX_FX16_TO_F32(native.aspectRatio),
        .startDelay = native.startDelay,
        .minRotation = native.minRotation,
        .maxRotation = native.maxRotation,
        .initAngle = native.initAngle,
        .emitterLifeTime = native.emitterLifeTime,
        .particleLifeTime = native.particleLifeTime,
        .randomAttenuation = {
            .baseScale = native.randomAttenuation.baseScale,
            .lifeTime = native.randomAttenuation.lifeTime,
            .initVel = native.randomAttenuation.initVel
        },
        .misc = {
            .emissionInterval = native.misc.emissionInterval,
            .baseAlpha = native.misc.baseAlpha,
            .airResistance = native.misc.airResistance,
            .textureIndex = native.misc.textureIndex,
            .loopFrames = native.misc.loopFrames,
            .dbbScale = native.misc.dbbScale,
            .textureTileCountS = native.misc.textureTileCountS,
            .textureTileCountT = native.misc.textureTileCountT,
            .scaleAnimDir = native.misc.scaleAnimDir,
            .dpolFaceEmitter = native.misc.dpolFaceEmitter,
            .flipTextureS = native.misc.flipTextureS,
            .flipTextureT = native.misc.flipTextureT
        },
        .polygonX = FX_FX16_TO_F32(native.polygonX),
        .polygonY = FX_FX16_TO_F32(native.polygonY),
    };
}

SPLScaleAnim SPLArchive::fromNative(const SPLScaleAnimNative &native) {
    return SPLScaleAnim {
        .start = FX_FX16_TO_F32(native.start),
        .mid = FX_FX16_TO_F32(native.mid),
        .end = FX_FX16_TO_F32(native.end),
        .curve = native.curve,
        .flags = { .loop = native.flags.loop }
    };
}

SPLColorAnim SPLArchive::fromNative(const SPLColorAnimNative &native) {
    return SPLColorAnim {
        .start = native.start.toVec3(),
        .end = native.end.toVec3(),
        .curve = native.curve,
        .flags = { 
            .randomStartColor = native.flags.randomStartColor,
            .loop = native.flags.loop,
            .interpolate = native.flags.interpolate
        }
    };
}

SPLAlphaAnim SPLArchive::fromNative(const SPLAlphaAnimNative &native) {
    return SPLAlphaAnim {
        .alpha = { .all = native.alpha.all },
        .flags = {
            .randomRange = native.flags.randomRange,
            .loop = native.flags.loop
        },
        .curve = native.curve
    };
}

SPLTexAnim SPLArchive::fromNative(const SPLTexAnimNative &native) {
    return SPLTexAnim {
        .textures = {
            native.textures[0],
            native.textures[1],
            native.textures[2],
            native.textures[3],
            native.textures[4],
            native.textures[5],
            native.textures[6],
            native.textures[7]
        },
        .param = {
            .frameCount = native.param.frameCount,
            .step = native.param.step,
            .randomizeInit = native.param.randomizeInit,
            .loop = native.param.loop
        }
    };
}

SPLChildResource SPLArchive::fromNative(const SPLChildResourceNative &native) {
    return SPLChildResource {
        .flags = native.flags,
        .randomInitVelMag = FX_FX16_TO_F32(native.randomInitVelMag),
        .endScale = FX_FX16_TO_F32(native.endScale),
        .lifeTime = native.lifeTime,
        .velocityRatio = native.velocityRatio,
        .scaleRatio = native.scaleRatio,
        .misc = {
            .emissionCount = native.misc.emissionCount,
            .emissionDelay = native.misc.emissionDelay,
            .emissionInterval = native.misc.emissionInterval,
            .texture = native.misc.texture,
            .textureTileCountS = native.misc.textureTileCountS,
            .textureTileCountT = native.misc.textureTileCountT,
            .flipTextureS = native.misc.flipTextureS,
            .flipTextureT = native.misc.flipTextureT,
            .dpolFaceEmitter = native.misc.dpolFaceEmitter
        }
    };
}

std::shared_ptr<SPLGravityBehavior> SPLArchive::fromNative(const SPLGravityBehaviorNative& native) {
    return std::make_shared<SPLGravityBehavior>(native);
}

std::shared_ptr<SPLRandomBehavior> SPLArchive::fromNative(const SPLRandomBehaviorNative& native) {
    return std::make_shared<SPLRandomBehavior>(native);
}

std::shared_ptr<SPLMagnetBehavior> SPLArchive::fromNative(const SPLMagnetBehaviorNative& native) {
    return std::make_shared<SPLMagnetBehavior>(native);
}

std::shared_ptr<SPLSpinBehavior> SPLArchive::fromNative(const SPLSpinBehaviorNative& native) {
    return std::make_shared<SPLSpinBehavior>(native);
}

std::shared_ptr<SPLCollisionPlaneBehavior> SPLArchive::fromNative(const SPLCollisionPlaneBehaviorNative& native) {
    return std::make_shared<SPLCollisionPlaneBehavior>(native);
}

std::shared_ptr<SPLConvergenceBehavior> SPLArchive::fromNative(const SPLConvergenceBehaviorNative& native) {
    return std::make_shared<SPLConvergenceBehavior>(native);
}

