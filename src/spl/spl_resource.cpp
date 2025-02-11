#include "spl_resource.h"


SPLResource SPLResource::create() {
    SPLResource res{};
    std::memset(&res.header, 0, sizeof(res.header));

    res.header.emissionCount = 1;
    res.header.color = { 1.0f, 1.0f, 1.0f };
    res.header.baseScale = 1.0f;
    res.header.aspectRatio = 1.0f;
    res.header.emitterLifeTime = 0.1f;
    res.header.particleLifeTime = 0.1f;
    res.header.variance.lifeTime = 1.0f;
    res.header.misc.emissionInterval = 0.1f;
    res.header.misc.baseAlpha = 1.0f;

    return res;
}
