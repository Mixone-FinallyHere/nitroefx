#include "spl_archive.h"
#include "gl_util.h"

#include <gl/glew.h>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <concepts>


template<class T> requires std::is_trivially_copyable_v<T>
std::istream& operator>>(std::istream& stream, T& v) {
    return stream.read(reinterpret_cast<char*>(&v), sizeof(T));
}


SPLArchive::SPLArchive(const std::filesystem::path& filename) : m_header() {
    load(filename);
}


void SPLArchive::load(const std::filesystem::path& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file) {
        spdlog::error("Failed to open file: {}", filename.string());
        return;
    }

    file >> m_header;

    m_resources.resize(m_header.resCount);
    
    for (size_t i = 0; i < m_header.resCount; i++) {
        SPLResource& res = m_resources[i];

        SPLResourceHeaderNative header;
        file >> header;

        res.header = fromNative(header);
        const auto& flags = res.header.flags;

        // Animations
        if (flags.hasScaleAnim) {
            SPLScaleAnimNative scaleAnim;
            file >> scaleAnim;
            res.scaleAnim = fromNative(scaleAnim);
        }

        if (flags.hasColorAnim) {
            SPLColorAnimNative colorAnim;
            file >> colorAnim;
            res.colorAnim = fromNative(colorAnim);
        }

        if (flags.hasAlphaAnim) {
            SPLAlphaAnimNative alphaAnim;
            file >> alphaAnim;
            res.alphaAnim = fromNative(alphaAnim);
        }

        if (flags.hasTexAnim) {
            SPLTexAnimNative texAnim;
            file >> texAnim;
            res.texAnim = fromNative(texAnim);
        }

        if (flags.hasChildResource) {
            SPLChildResourceNative childResource;
            file >> childResource;
            res.childResource = fromNative(childResource);
        }

        // Behaviors
        if (flags.hasGravityBehavior) {
            SPLGravityBehaviorNative gravityBehavior;
            file >> gravityBehavior;
            res.behaviors.push_back(fromNative(gravityBehavior));
        }

        if (flags.hasRandomBehavior) {
            SPLRandomBehaviorNative randomBehavior;
            file >> randomBehavior;
            res.behaviors.push_back(fromNative(randomBehavior));
        }

        if (flags.hasMagnetBehavior) {
            SPLMagnetBehaviorNative magnetBehavior;
            file >> magnetBehavior;
            res.behaviors.push_back(fromNative(magnetBehavior));
        }

        if (flags.hasSpinBehavior) {
            SPLSpinBehaviorNative spinBehavior;
            file >> spinBehavior;
            res.behaviors.push_back(fromNative(spinBehavior));
        }

        if (flags.hasCollisionPlaneBehavior) {
            SPLCollisionPlaneBehaviorNative collisionPlaneBehavior;
            file >> collisionPlaneBehavior;
            res.behaviors.push_back(fromNative(collisionPlaneBehavior));
        }

        if (flags.hasConvergenceBehavior) {
            SPLConvergenceBehaviorNative convergenceBehavior;
            file >> convergenceBehavior;
            res.behaviors.push_back(fromNative(convergenceBehavior));
        }
    }

    m_textures.resize(m_header.texCount);

    for (size_t i = 0; i < m_header.texCount; i++) {
        SPLTexture& tex = m_textures[i];

        SPLTextureResource texRes;
        s64 offset = file.tellg();
        file >> texRes;

        tex.resource = nullptr;
        tex.param = fromNative(texRes.param);
        tex.width = 1 << (texRes.param.s + 3);
        tex.height = 1 << (texRes.param.t + 3);

        if (!texRes.param.useSharedTexture) { // Handle shared textures later
            if (texRes.textureSize > 0) {
                m_textureData.emplace_back(texRes.textureSize);
                file.read((char*)m_textureData.back().data(), texRes.textureSize);
            }

            if (texRes.textureSize > 0) {
                m_paletteData.emplace_back(texRes.paletteSize);
                file.seekg(offset + texRes.paletteOffset, std::ios::beg);
                file.read((char*)m_paletteData.back().data(), texRes.paletteSize);
            } else {
                // No palette data (TextureFormat::Direct)
                m_paletteData.emplace_back();
            }

            tex.textureData = m_textureData.back();
            tex.paletteData = m_paletteData.back();

            tex.glTexture = std::make_shared<GLTexture>(tex);
        }

        file.seekg(offset + texRes.resourceSize, std::ios::beg);
    }

    // Resolve shared textures
    for (auto& tex : m_textures) {
        if (tex.param.useSharedTexture) {
            tex.textureData = m_textureData[tex.param.sharedTexID];
            tex.paletteData = m_paletteData[tex.param.sharedTexID];
            tex.glTexture = m_textures[tex.param.sharedTexID].glTexture;
        }
    }
}

SPLResourceHeader SPLArchive::fromNative(const SPLResourceHeaderNative &native) {
    return SPLResourceHeader {
        .flags = {
            .emissionType = (SPLEmissionType)native.flags.emissionType,
            .drawType = (SPLDrawType)native.flags.drawType,
            .emissionAxis = (SPLEmissionAxis)native.flags.circleAxis,
            .hasScaleAnim = !!native.flags.hasScaleAnim,
            .hasColorAnim = !!native.flags.hasColorAnim,
            .hasAlphaAnim = !!native.flags.hasAlphaAnim,
            .hasTexAnim = !!native.flags.hasTexAnim,
            .hasRotation = !!native.flags.hasRotation,
            .randomInitAngle = !!native.flags.randomInitAngle,
            .selfMaintaining = !!native.flags.selfMaintaining,
            .followEmitter = !!native.flags.followEmitter,
            .hasChildResource = !!native.flags.hasChildResource,
            .polygonRotAxis = (SPLPolygonRotAxis)native.flags.polygonRotAxis,
            .polygonReferencePlane = (u8)native.flags.polygonReferencePlane,
            .randomizeLoopedAnim = !!native.flags.randomizeLoopedAnim,
            .drawChildrenFirst = !!native.flags.drawChildrenFirst,
            .hideParent = !!native.flags.hideParent,
            .useViewSpace = !!native.flags.useViewSpace,
            .hasGravityBehavior = !!native.flags.hasGravityBehavior,
            .hasRandomBehavior = !!native.flags.hasRandomBehavior,
            .hasMagnetBehavior = !!native.flags.hasMagnetBehavior,
            .hasSpinBehavior = !!native.flags.hasSpinBehavior,
            .hasCollisionPlaneBehavior = !!native.flags.hasCollisionPlaneBehavior,
            .hasConvergenceBehavior = !!native.flags.hasConvergenceBehavior,
            .hasFixedPolygonID = !!native.flags.hasFixedPolygonID,
            .childHasFixedPolygonID = !!native.flags.childHasFixedPolygonID
        },
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
        .startDelay = toSeconds(native.startDelay),
        .minRotation = toAngle(native.minRotation),
        .maxRotation = toAngle(native.maxRotation),
        .initAngle = toAngle(native.initAngle),
        .emitterLifeTime = toSeconds(native.emitterLifeTime),
        .particleLifeTime = toSeconds(native.particleLifeTime),
        .variance = {
            .baseScale = (f32)native.variance.baseScale / 255.0f,
            .lifeTime = (f32)native.variance.lifeTime / 255.0f,
            .initVel = (f32)native.variance.initVel / 255.0f
        },
        .misc = {
            .emissionInterval = toSeconds(native.misc.emissionInterval),
            .baseAlpha = native.misc.baseAlpha / 31.0f,
            // 0 - 255 -> 0.75 - 1.25 (almost)
            .airResistance = 0.75f + (f32)native.misc.airResistance / 256.0f * 0.5f,
            .textureIndex = (u8)native.misc.textureIndex,
            .loopTime = toSeconds(native.misc.loopFrames),
            .dbbScale = (u16)native.misc.dbbScale,
            .textureTileCountS = (u8)native.misc.textureTileCountS,
            .textureTileCountT = (u8)native.misc.textureTileCountT,
            .scaleAnimDir = (SPLScaleAnimDir)native.misc.scaleAnimDir,
            .dpolFaceEmitter = !!native.misc.dpolFaceEmitter,
            .flipTextureS = !!native.misc.flipTextureS,
            .flipTextureT = !!native.misc.flipTextureT
        },
        .polygonX = FX_FX16_TO_F32(native.polygonX),
        .polygonY = FX_FX16_TO_F32(native.polygonY),
    };
}

SPLScaleAnim SPLArchive::fromNative(const SPLScaleAnimNative &native) {
    return SPLScaleAnim(native);
}

SPLColorAnim SPLArchive::fromNative(const SPLColorAnimNative &native) {
    return SPLColorAnim(native);
}

SPLAlphaAnim SPLArchive::fromNative(const SPLAlphaAnimNative &native) {
    return SPLAlphaAnim(native);
}

SPLTexAnim SPLArchive::fromNative(const SPLTexAnimNative &native) {
    return SPLTexAnim(native);
}

SPLChildResource SPLArchive::fromNative(const SPLChildResourceNative &native) {
    return SPLChildResource {
        .flags = {
            .usesBehaviors = !!native.flags.usesBehaviors,
            .hasScaleAnim = !!native.flags.hasScaleAnim,
            .hasAlphaAnim = !!native.flags.hasAlphaAnim,
            .rotationType = (SPLChildRotationType)native.flags.rotationType,
            .followEmitter = !!native.flags.followEmitter,
            .useChildColor = !!native.flags.useChildColor,
            .drawType = (SPLDrawType)native.flags.drawType,
            .polygonRotAxis = (SPLPolygonRotAxis)native.flags.polygonRotAxis,
            .polygonReferencePlane = (u8)native.flags.polygonReferencePlane,
        },
        .randomInitVelMag = FX_FX16_TO_F32(native.randomInitVelMag),
        .endScale = FX_FX16_TO_F32(native.endScale),
        .lifeTime = toSeconds(native.lifeTime),
        .velocityRatio = toSeconds(native.velocityRatio),
        .scaleRatio = toSeconds(native.scaleRatio),
        .misc = {
            .emissionCount = (u8)native.misc.emissionCount,
            .emissionDelay = (f32)native.misc.emissionDelay / 255.0f,
            .emissionInterval = toSeconds(native.misc.emissionInterval),
            .texture = (u8)native.misc.texture,
            .textureTileCountS = (u8)native.misc.textureTileCountS,
            .textureTileCountT = (u8)native.misc.textureTileCountT,
            .flipTextureS = !!native.misc.flipTextureS,
            .flipTextureT = !!native.misc.flipTextureT,
            .dpolFaceEmitter = !!native.misc.dpolFaceEmitter
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

SPLTextureParam SPLArchive::fromNative(const SPLTextureParamNative& native) {
    return SPLTextureParam{
        .format = (TextureFormat)native.format,
        .s = (u8)native.s,
        .t = (u8)native.t,
        .repeat = (TextureRepeat)native.repeat,
        .flip = (TextureFlip)native.flip,
        .palColor0Transparent = !!native.palColor0,
        .useSharedTexture = !!native.useSharedTexture,
        .sharedTexID = (u8)native.sharedTexID
    };
}

