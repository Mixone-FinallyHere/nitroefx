#pragma once
#include "spl/spl_resource.h"
#include "editor_instance.h"
#include "grid_renderer.h"
#include "types.h"

#include <imgui.h>

#include <array>
#include <chrono>
#include <unordered_map>
#include <vector>

enum class EmitterSpawnType {
    SingleShot,
    Looped,
    Interval
};

class Editor {
public:
    Editor();

    void render();
    void renderParticles();
    void openPicker();
    void openEditor();
    void updateParticles(float deltaTime);

    void playEmitterAction(EmitterSpawnType spawnType);
    void killEmitters();
    void resetCamera();

    void handleEvent(const SDL_Event& event);

private:
    void renderResourcePicker();
    void renderTextureManager();
    void renderResourceEditor();

    void renderHeaderEditor(SPLResourceHeader& header) const;
    void renderBehaviorEditor(SPLResource& res);

    bool renderGravityBehaviorEditor(const std::shared_ptr<SPLGravityBehavior>& gravity);
    bool renderRandomBehaviorEditor(const std::shared_ptr<SPLRandomBehavior>& random);
    bool renderMagnetBehaviorEditor(const std::shared_ptr<SPLMagnetBehavior>& magnet);
    bool renderSpinBehaviorEditor(const std::shared_ptr<SPLSpinBehavior>& spin);
    bool renderCollisionPlaneBehaviorEditor(const std::shared_ptr<SPLCollisionPlaneBehavior>& collisionPlane);
    bool renderConvergenceBehaviorEditor(const std::shared_ptr<SPLConvergenceBehavior>& convergence);

    void renderAnimationEditor(SPLResource& res);
    bool renderScaleAnimEditor(SPLScaleAnim& res);
    bool renderColorAnimEditor(const SPLResource& mainRes, SPLColorAnim& res);
    bool renderAlphaAnimEditor(SPLAlphaAnim& res);
    bool renderTexAnimEditor(SPLTexAnim& res);

    void renderChildrenEditor(SPLResource& res);

    void openTempTexture(std::string_view path);
    void discardTempTexture();

private:
    struct TempTexture {
        std::string path;
        u8* data;
        s32 width;
        s32 height;
        s32 channels;
        TextureFormat format;
        bool requiresColorCompression;
        bool requiresAlphaCompression;
        TextureConversionPreference preference;
    };

    bool m_pickerOpen = true;
    bool m_textureManagerOpen = true;
    bool m_editorOpen = true;
    float m_timeScale = 1.0f;

    EmitterSpawnType m_emitterSpawnType = EmitterSpawnType::SingleShot;
    float m_emitterInterval = 1.0f; // seconds

    static inline const u32 s_hoverAccentColor = ImGui::ColorConvertFloat4ToU32({ 0.7f, 0.3f, 0.7f, 1.0f });
    static constexpr glm::ivec2 s_gridDimensions = { 20, 20 };
    static constexpr glm::vec2 s_gridSpacing = { 1.0f, 1.0f };

    std::array<f32, 64> m_xAnimBuffer;
    std::array<f32, 64> m_yAnimBuffer;

    TempTexture* m_tempTexture = nullptr;

    std::unordered_map<u64, size_t> m_selectedResources;
    std::weak_ptr<EditorInstance> m_activeEditor;
    std::shared_ptr<GridRenderer> m_gridRenderer;

    struct EmitterSpawnTask {
        u64 resourceIndex;
        std::chrono::time_point<std::chrono::steady_clock> time;
        std::chrono::duration<float> interval;
        u64 editorID;
    };

    std::vector<EmitterSpawnTask> m_emitterTasks;
};
