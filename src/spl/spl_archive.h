#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "spl_resource.h"


class SPLArchive {
public:
    explicit SPLArchive(const std::filesystem::path& filename);

    const SPLResource& getResource(size_t index) const { return m_resources[index]; }

    const std::vector<SPLResource>& getResources() const { return m_resources; }

    const SPLTexture& getTexture(size_t index) const { return m_textures[index]; }
    const std::vector<SPLTexture>& getTextures() const { return m_textures; }

    size_t getResourceCount() const { return m_resources.size(); }
    size_t getTextureCount() const { return m_header.texCount; }

private:
    void load(const std::filesystem::path& filename);

    static SPLResourceHeader fromNative(const SPLResourceHeaderNative& native);

    SPLScaleAnim fromNative(const SPLScaleAnimNative& native);
    SPLColorAnim fromNative(const SPLColorAnimNative& native);
    SPLAlphaAnim fromNative(const SPLAlphaAnimNative& native);
    SPLTexAnim fromNative(const SPLTexAnimNative& native);
    SPLChildResource fromNative(const SPLChildResourceNative& native);

    std::shared_ptr<SPLGravityBehavior> fromNative(const SPLGravityBehaviorNative& native);
    std::shared_ptr<SPLRandomBehavior> fromNative(const SPLRandomBehaviorNative& native);
    std::shared_ptr<SPLMagnetBehavior> fromNative(const SPLMagnetBehaviorNative& native);
    std::shared_ptr<SPLSpinBehavior> fromNative(const SPLSpinBehaviorNative& native);
    std::shared_ptr<SPLCollisionPlaneBehavior> fromNative(const SPLCollisionPlaneBehaviorNative& native);
    std::shared_ptr<SPLConvergenceBehavior> fromNative(const SPLConvergenceBehaviorNative& native);

    SPLTextureParam fromNative(const SPLTextureParamNative& native);

private:
    SPLFileHeader m_header;
    std::vector<SPLResource> m_resources;
    std::vector<SPLTexture> m_textures;
    std::vector<std::vector<u8>> m_textureData;
    std::vector<std::vector<u8>> m_paletteData;
};

