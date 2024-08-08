#include "spl_resource.h"
#include "spl_particle.h"

#include <glm/common.hpp>

#include "random.h"


void SPLScaleAnim::apply(SPLParticle& ptcl, SPLResource& resource, f32 lifeRate) {
    const f32 in = curve.getIn();
    const f32 out = curve.getOut();

    if (lifeRate < in) {
        ptcl.animScale = glm::mix(start, mid, lifeRate / in);
    } else if (lifeRate < out) {
        ptcl.animScale = mid;
    } else {
        ptcl.animScale = glm::mix(mid, end, (lifeRate - out) / (1.0f - out));
    }
}

void SPLColorAnim::apply(SPLParticle& ptcl, SPLResource& resource, f32 lifeRate) {
    const float in = curve.getIn();
    const float peak = curve.getPeak();
    const float out = curve.getOut();

    if (lifeRate < in) {
        ptcl.color = start;
    } else if (lifeRate < peak) {
        if (flags.interpolate) {
            ptcl.color = glm::mix(start, resource.header.color, (lifeRate - in) / (peak - in));
        } else {
            ptcl.color = resource.header.color;
        }
    } else if (lifeRate < out) {
        if (flags.interpolate) {
            ptcl.color = glm::mix(resource.header.color, end, (lifeRate - peak) / (out - peak));
        } else {
            ptcl.color = end;
        }
    } else {
        ptcl.color = end;
    }
}

void SPLAlphaAnim::apply(SPLParticle& ptcl, SPLResource& resource, f32 lifeRate) {
    const f32 in = curve.getIn();
    const f32 out = curve.getOut();

    if (lifeRate < in) {
        ptcl.visibility.animAlpha = glm::mix(alpha.start, alpha.mid, lifeRate / in);
    } else if (lifeRate < out) {
        ptcl.visibility.animAlpha = alpha.mid;
    } else {
        ptcl.visibility.animAlpha = glm::mix(alpha.mid, alpha.end, (lifeRate - out) / (1.0f - out));
    }

    ptcl.visibility.animAlpha = glm::clamp(
        random::scaledRange(ptcl.visibility.animAlpha, flags.randomRange), 
        0.0f, 1.0f
    );
}

void SPLTexAnim::apply(SPLParticle& ptcl, SPLResource& resource, f32 lifeRate) {
    
    for (int i = 0; i < param.textureCount; i++) {
        if (lifeRate < param.step * (i + 1)) {
            ptcl.texture = textures[i];
            break;
        }
    }
}

void SPLChildResource::applyScaleAnim(SPLParticle& ptcl, f32 lifeRate) {
    ptcl.animScale = glm::mix(0.0f, endScale, lifeRate); // scale up
}

void SPLChildResource::applyAlphaAnim(SPLParticle& ptcl, f32 lifeRate) {
    ptcl.visibility.animAlpha = glm::mix(1.0f, 0.0f, lifeRate); // fade out
}
