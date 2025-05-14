#include "editor.h"
#include "project_manager.h"
#include "spl/enum_names.h"
#include "help_messages.h"

#include <array>
#include <ranges>
#include <fmt/format.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/integer.hpp>
#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>
#include <tinyfiledialogs.h>
#include <stb_image.h>
#include <libimagequant.h>

#define LOCKED_EDITOR() activeEditor_locked
#define LOCK_EDITOR() auto LOCKED_EDITOR() = m_activeEditor.lock()
#define NOTIFY(action) LOCKED_EDITOR()->valueChanged(action)
#define HELP(name) helpPopup(help::name)


namespace {

constexpr std::array s_emitterSpawnTypes = {
    "Single Shot",
    "Looped",
    "Interval"
};

}


Editor::Editor() : m_xAnimBuffer(), m_yAnimBuffer() {
    m_gridRenderer = std::make_shared<GridRenderer>(s_gridDimensions, s_gridSpacing);
    m_debugRenderer = std::make_unique<DebugRenderer>(1000);
    m_collisionGridRenderer = std::make_shared<GridRenderer>(s_gridDimensions / 2, s_gridSpacing);
}

void Editor::render() {
    const auto& instances = g_projectManager->getOpenEditors();

    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar
        | ImGuiDockNodeFlags_NoDockingOverCentralNode
        | ImGuiDockNodeFlags_NoUndocking;
        ImGui::SetNextWindowClass(&windowClass);

    ImGui::Begin("Work Area##Editor", nullptr,
        // ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration
    );

    std::vector<std::shared_ptr<EditorInstance>> toClose;
    if (ImGui::BeginTabBar("Editor Instances", ImGuiTabBarFlags_Reorderable 
        | ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        for (const auto& instance : instances) {
            const auto [open, active] = instance->render();
            if (!open) {
                toClose.push_back(instance);
            }
            if (active) {
                g_projectManager->setActiveEditor(instance);
            }
        }

        ImGui::EndTabBar();
    }

    for (const auto& instance : toClose) {
        g_projectManager->closeEditor(instance);
    }

    ImGui::End();

    if (m_pickerOpen) {
        renderResourcePicker();
    }

    if (m_textureManagerOpen) {
        renderTextureManager();
    }

    if (m_editorOpen) {
        renderResourceEditor();
    }
}

void Editor::renderParticles() {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    std::vector<Renderer*> renderers = { m_gridRenderer.get(), m_debugRenderer.get() };

    const auto& archive = editor->getArchive();
    const auto& resources = archive.getResources();
    if (m_selectedResources[editor->getUniqueID()] != -1) {
        const auto& resource = resources[m_selectedResources[editor->getUniqueID()]];
        for (const auto& bhv : resource.behaviors) {
            if (bhv->type == SPLBehaviorType::CollisionPlane) {
                const auto colPlane = std::dynamic_pointer_cast<SPLCollisionPlaneBehavior>(bhv);
                const glm::vec4 color = colPlane->collisionType == SPLCollisionType::Kill ? glm::vec4(1, 0, 0, 0.5f) : glm::vec4(0, 1, 0, 0.5f);
                m_collisionGridRenderer->setColor(color);
                m_collisionGridRenderer->setHeight(colPlane->y);
                renderers.push_back(m_collisionGridRenderer.get());
            }
        }
    }

    editor->renderParticles(renderers);
}

void Editor::openPicker() {
    m_pickerOpen = true;
}

void Editor::openEditor() {
    m_editorOpen = true;
}

void Editor::updateParticles(float deltaTime) {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    for (auto& task : m_emitterTasks) {
        const auto now = std::chrono::steady_clock::now();
        if (task.editorID == editor->getUniqueID() && m_timeScale * (now - task.time) >= task.interval) {
            editor->getParticleSystem().addEmitter(
                editor->getArchive().getResources()[task.resourceIndex], 
                false
            );
            task.time = now;
        }
    }

    editor->updateParticles(deltaTime * m_timeScale);
}

void Editor::save() {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->save();
}

void Editor::saveAs(const std::filesystem::path& path) {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->saveAs(path);
}

void Editor::playEmitterAction(EmitterSpawnType spawnType) {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    const auto resourceIndex = m_selectedResources[editor->getUniqueID()];
    editor->getParticleSystem().addEmitter(
        editor->getArchive().getResource(resourceIndex),
        spawnType == EmitterSpawnType::Looped
    );

    if (spawnType == EmitterSpawnType::Interval) {
        m_emitterTasks.emplace_back(
            resourceIndex,
            std::chrono::steady_clock::now(),
            std::chrono::duration<float>(m_emitterInterval),
            editor->getUniqueID()
        );
    }
}

void Editor::killEmitters() {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->getParticleSystem().killAllEmitters();
    std::erase_if(m_emitterTasks, [id = editor->getUniqueID()](const auto& task) {
        return task.editorID == id;
    });
}

void Editor::resetCamera() {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->getCamera().reset();
}

void Editor::handleEvent(const SDL_Event& event) {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->handleEvent(event);
}

void Editor::renderResourcePicker() {
    if (ImGui::Begin("Resource Picker##Editor", &m_pickerOpen)) {

        const auto& editor = g_projectManager->getActiveEditor();
        if (!editor) {
            ImGui::Text("No editor open");
            ImGui::End();
            return;
        }

        auto& archive = editor->getArchive();
        auto& resources = archive.getResources();
        auto& textures = archive.getTextures();

        const auto id = editor->getUniqueID();
        if (!m_selectedResources.contains(id)) {
            m_selectedResources[id] = -1;
        }

        const auto contentRegion = ImGui::GetContentRegionAvail();
        if (ImGui::BeginListBox("##Resources", contentRegion)) {
            const ImGuiStyle& style = ImGui::GetStyle();

            for (size_t i = 0; i < resources.size(); ++i) {
                const auto& resource = resources[i];
                const auto& texture = textures[resource.header.misc.textureIndex];

                ImGui::PushID(i);
                const auto name = fmt::format("[{}] Tex {}x{}", i, texture.width, texture.height);

                auto bgColor = m_selectedResources[id] == i
                    ? style.Colors[ImGuiCol_ButtonActive]
                    : style.Colors[ImGuiCol_Button];

                const auto cursor = ImGui::GetCursorScreenPos();
                if (ImGui::InvisibleButton("##Resource", { contentRegion.x, 32 })) {
                    m_selectedResources[id] = i;
                }

                if (ImGui::IsItemHovered()) {
                    bgColor = style.Colors[ImGuiCol_ButtonHovered];
                }

                // Draw a filled rectangle behind the item
                ImGui::GetWindowDrawList()->AddRectFilled(
                    cursor, 
                    { cursor.x + contentRegion.x, cursor.y + 32 }, 
                    ImGui::ColorConvertFloat4ToU32(bgColor),
                    2.5f
                );

                ImGui::SetCursorScreenPos(cursor);
                ImGui::Image((ImTextureID)(uintptr_t)texture.glTexture->getHandle(), { 32, 32 });

                ImGui::SameLine();

                const auto textHeight = ImGui::GetFontSize();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (32 - textHeight) / 2);
                ImGui::TextUnformatted(name.c_str());

                ImGui::PopID();
            }

            ImGui::EndListBox();
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("##AddResourcePopup");
        }

        if (ImGui::BeginPopup("##AddResourcePopup")) {
            if (ImGui::MenuItem("Add Resource")) {
                resources.emplace_back(SPLResource::create());
                m_selectedResources[id] = resources.size() - 1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void Editor::renderTextureManager() {
    if (ImGui::Begin("Texture Manager##Editor", &m_textureManagerOpen)) {
        const auto& editor = g_projectManager->getActiveEditor();
        if (!editor) {
            ImGui::Text("No editor open");
            ImGui::End();
            return;
        }

        auto& archive = editor->getArchive();
        auto& textures = archive.getTextures();

        if (ImGui::Button("Import")) {
            const auto path = tinyfd_openFileDialog(
                "Import Texture", 
                "", 
                0, 
                nullptr, 
                "Image Files", 
                0
            );
            if (path) {
                openTempTexture(path);
                ImGui::OpenPopup("##ImportTexturePopup");
            }
        }

        for (int i = 0; i < textures.size(); ++i) {
            auto& texture = textures[i];
            const auto name = fmt::format("[{}] Tex {}x{}", i, texture.width, texture.height);

            ImGui::Image((ImTextureID)texture.glTexture->getHandle(), { 32, 32 });
            ImGui::SameLine();
            if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth)) {
                ImGui::InputScalar("S", ImGuiDataType_U8, &texture.param.s);
                ImGui::InputScalar("T", ImGuiDataType_U8, &texture.param.t);
                
                if (ImGui::BeginCombo("Repeat", getTextureRepeat(texture.param.repeat))) {
                    for (const auto [val, name] : detail::g_textureRepeatNames) {
                        if (editor->valueChanged(ImGui::Selectable(name, texture.param.repeat == val))) {
                            texture.param.repeat = val;
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Flip", getTextureFlip(texture.param.flip))) {
                    for (const auto [val, name] : detail::g_textureFlipNames) {
                        if (editor->valueChanged(ImGui::Selectable(name, texture.param.flip == val))) {
                            texture.param.flip = val;
                        }
                    }

                    ImGui::EndCombo();
                }

                editor->valueChanged(ImGui::Checkbox("Palette Color 0 Transparent", &texture.param.palColor0Transparent));
                editor->valueChanged(ImGui::Checkbox("Use Shared Texture", &texture.param.useSharedTexture));

                if (texture.param.useSharedTexture) {
                    ImGui::InputScalar("Shared Texture ID", ImGuiDataType_U8, &texture.param.sharedTexID);
                }

                ImGui::TreePop();
            }
        }

        if (ImGui::BeginPopup("##ImportTexturePopup")) {
            ImGui::Text("Texture Info:");
            ImGui::Text("Size: %dx%d", m_tempTexture->width, m_tempTexture->height);
            ImGui::Text("Channels: %d", m_tempTexture->channels);
            ImGui::Text("Number of unique Colors: %llu", m_tempTexture->suggestedSpec.uniqueColors.size());
            ImGui::Text("Number of unique Alphas: %llu", m_tempTexture->suggestedSpec.uniqueAlphas.size());

            const auto estimatedSize = m_tempTexture->suggestedSpec.getSizeEstimate(m_tempTexture->width, m_tempTexture->height);
            if (estimatedSize >= 1024) {
                ImGui::Text("Estimated Size: %zu kB", estimatedSize / 1024);
            } else {
                ImGui::Text("Estimated Size: %zu B", estimatedSize);
            }

            //ImGui::Text("Suggested Format: %s", getTextureFormat(m_tempTexture->suggestedSpec.format));
            if (ImGui::BeginCombo("Format", getTextureFormat(m_tempTexture->suggestedSpec.format))) {
                for (auto i = (int)TextureFormat::A3I5; i < (int)TextureFormat::Count; i++) {
                    if (ImGui::Selectable(getTextureFormat((TextureFormat)i), (int)m_tempTexture->suggestedSpec.format == i)) {
                        m_tempTexture->suggestedSpec.setFormat((TextureFormat)i);
                        quantizeTexture(
                            m_tempTexture->data,
                            m_tempTexture->width,
                            m_tempTexture->height,
                            m_tempTexture->suggestedSpec,
                            m_tempTexture->quantized
                        );

                        m_tempTexture->texture->update(m_tempTexture->quantized);
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::Text("Color Compression: %s", m_tempTexture->suggestedSpec.requiresColorCompression ? "Yes" : "No");
            ImGui::Text("Alpha Compression: %s", m_tempTexture->suggestedSpec.requiresAlphaCompression ? "Yes" : "No");

            ImGui::Image((ImTextureID)m_tempTexture->texture->getHandle(), { 256, 256 });

            if (ImGui::Button("Confirm")) {
                importTempTexture();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                discardTempTexture();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void Editor::renderResourceEditor() {
    if (ImGui::Begin("Resource Editor##Editor", &m_editorOpen)) {
        ImGui::SliderFloat("Global Time Scale", &m_timeScale, 0.0f, 2.0f, "%.2f");

        const auto& editor = g_projectManager->getActiveEditor();
        if (!editor) {
            ImGui::Text("No editor open");
            ImGui::End();
            return;
        }

        m_activeEditor = editor;

        auto& archive = editor->getArchive();
        auto& resources = archive.getResources();
        auto& textures = archive.getTextures();

        const auto id = editor->getUniqueID();
        if (!m_selectedResources.contains(id)) {
            m_selectedResources[id] = -1;
        }

        if (m_selectedResources[id] != -1) {
            auto& resource = resources[m_selectedResources[id]];
            const auto& texture = textures[resource.header.misc.textureIndex];

            if (ImGui::Button("Play Emitter")) {
                playEmitterAction(m_emitterSpawnType);
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            ImGui::Combo("##SpawnType", (int*)&m_emitterSpawnType, s_emitterSpawnTypes.data(), s_emitterSpawnTypes.size());

            if (m_emitterSpawnType == EmitterSpawnType::Interval) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputFloat("##Interval", &m_emitterInterval, 0.1f, 1.0f, "%.2fs");
            }

            if (ImGui::Button("Kill Emitters")) {
                killEmitters();
            }

            if (ImGui::BeginTabBar("##editorTabs")) {
                if (ImGui::BeginTabItem("General")) {
                    ImGui::BeginChild("##headerEditor", {}, ImGuiChildFlags_Border);
                    renderHeaderEditor(resource.header);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Behaviors")) {
                    ImGui::BeginChild("##headerEditor", {}, ImGuiChildFlags_Border);
                    renderBehaviorEditor(resource);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Animations")) {
                    ImGui::BeginChild("##animationEditor", {}, ImGuiChildFlags_Border);
                    renderAnimationEditor(resource);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Children")) {
                    ImGui::BeginChild("##childEditor", {}, ImGuiChildFlags_Border);
                    renderChildrenEditor(resource);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
    }

    ImGui::End();

    m_activeEditor.reset();
}

void Editor::renderHeaderEditor(SPLResourceHeader& header) const {
    if (m_activeEditor.expired()) {
        return;
    }

    LOCK_EDITOR();

    constexpr auto helpPopup = [](std::string_view text) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.data(), text.data() + text.size());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    };

    auto& flags = header.flags;
    auto& misc = header.misc;
    constexpr f32 frameTime = 1.0f / (f32)SPLArchive::SPL_FRAMES_PER_SECOND;

    auto open = ImGui::TreeNodeEx("##emitterSettings", ImGuiTreeNodeFlags_SpanAvailWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
    ImGui::SeparatorText("Emitter Settings");
    if (open) {
        if (ImGui::BeginCombo("Emission Type", getEmissionType(flags.emissionType))) {
            for (const auto [val, name] : detail::g_emissionTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.emissionType == val))) {
                    flags.emissionType = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(emissionType);

        if (ImGui::BeginCombo("Emission Axis", getEmissionAxis(flags.emissionAxis))) {
            for (const auto [val, name] : detail::g_emissionAxisNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.emissionAxis == val))) {
                    flags.emissionAxis = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(emissionAxis);

        NOTIFY(ImGui::Checkbox("Self Maintaining", &flags.selfMaintaining));
        HELP(selfMaintaining);

        NOTIFY(ImGui::Checkbox("Draw Children First", &flags.drawChildrenFirst));
        HELP(drawChildrenFirst);

        NOTIFY(ImGui::Checkbox("Hide Parent", &flags.hideParent));
        HELP(hideParent);

        NOTIFY(ImGui::Checkbox("Use View Space", &flags.useViewSpace));
        HELP(useViewSpace);

        NOTIFY(ImGui::Checkbox("Has Fixed Polygon ID", &flags.hasFixedPolygonID));
        HELP(hasFixedPolygonID);

        NOTIFY(ImGui::Checkbox("Child Fixed Polygon ID", &flags.childHasFixedPolygonID));
        HELP(childHasFixedPolygonID);

        NOTIFY(ImGui::DragFloat3("Emitter Base Pos", glm::value_ptr(header.emitterBasePos), 0.01f));
        HELP(emitterBasePos);

        NOTIFY(ImGui::SliderFloat("Lifetime", &header.emitterLifeTime, frameTime, 60, "%.4fs", ImGuiSliderFlags_Logarithmic));
        HELP(emitterLifeTime);

        NOTIFY(ImGui::DragInt("Emission Amount", (int*)&header.emissionCount, 1, 0, 20));
        HELP(emissionCount);

        // The in-file value is 8 bits, 255 = 255 frames of delay, 255f / 30fps = 8.5s
        NOTIFY(ImGui::SliderFloat("Emission Interval", &misc.emissionInterval, frameTime, 8.5f, "%.4fs"));
        HELP(emissionInterval);

        u32 emissions = (u32)glm::ceil(header.emitterLifeTime / misc.emissionInterval);
        const u32 maxEmissions = (u32)(header.emitterLifeTime / frameTime);
        if (NOTIFY(ImGui::SliderInt("Emissions", (int*)&emissions, 1, maxEmissions))) {
            misc.emissionInterval = header.emitterLifeTime / (f32)emissions;
        }
        HELP(emissions);

        NOTIFY(ImGui::SliderFloat("Start Delay", &header.startDelay, 0, header.emitterLifeTime, "%.2fs"));
        HELP(startDelay);

        NOTIFY(ImGui::SliderFloat("Radius", &header.radius, 0.01f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(radius);

        NOTIFY(ImGui::SliderFloat("Length", &header.length, 0.01f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(length);

        NOTIFY(ImGui::DragFloat3("Axis", glm::value_ptr(header.axis)));
        HELP(axis);

        ImGui::TreePop();
    }

    open = ImGui::TreeNodeEx("##particleSettings", ImGuiTreeNodeFlags_SpanAvailWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
    ImGui::SeparatorText("Particle Settings");
    if (open) {
        if (ImGui::BeginCombo("Draw Type", getDrawType(flags.drawType))) {
            for (const auto [val, name] : detail::g_drawTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.drawType == val))) {
                    flags.drawType = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(drawType);

        const auto& textures = LOCKED_EDITOR()->getArchive().getTextures();
        if (ImGui::ImageButton((ImTextureID)textures[header.misc.textureIndex].glTexture->getHandle(), { 32, 32 })) {
            ImGui::OpenPopup(ImGui::GetID("##texturePicker"));
        }
        ImGui::SameLine();
        ImGui::Text("Texture");
        HELP(texture);

        NOTIFY(ImGui::Checkbox("Rotate", &flags.hasRotation));
        HELP(hasRotation);

        NOTIFY(ImGui::Checkbox("Random Init Angle", &flags.randomInitAngle));
        HELP(randomInitAngle);

        NOTIFY(ImGui::Checkbox("Follow Emitter", &flags.followEmitter));
        HELP(followEmitter);

        if (ImGui::BeginCombo("Polygon Rotation Axis", getPolygonRotAxis(flags.polygonRotAxis))) {
            for (const auto [val, name] : detail::g_polygonRotAxisNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.polygonRotAxis == val))) {
                    flags.polygonRotAxis = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(polygonRotAxis);

        ImGui::TextUnformatted("Polygon Reference Plane");
        HELP(polygonReferencePlane);
        ImGui::Indent();
        NOTIFY(ImGui::RadioButton("XY", &flags.polygonReferencePlane, 0));
        NOTIFY(ImGui::RadioButton("XZ", &flags.polygonReferencePlane, 1));
        ImGui::Unindent();

        NOTIFY(ImGui::ColorEdit3("Color", glm::value_ptr(header.color)));
        HELP(color);

        NOTIFY(ImGui::SliderFloat("Base Scale", &header.baseScale, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(baseScale);

        NOTIFY(ImGui::SliderAngle("Init Angle", &header.initAngle, 0));
        HELP(initAngle);

        NOTIFY(ImGui::SliderFloat("Base Alpha", &misc.baseAlpha, 0, 1));
        HELP(baseAlpha);

        NOTIFY(ImGui::SliderFloat("Lifetime", &header.particleLifeTime, frameTime, 60, "%.4fs", ImGuiSliderFlags_Logarithmic));
        HELP(particleLifeTime);

        NOTIFY(ImGui::DragFloat("Aspect Ratio", &header.aspectRatio, 0.05f));
        HELP(aspectRatio);

        NOTIFY(ImGui::DragFloat("Init Velocity Pos Amplifier", &header.initVelPosAmplifier, 0.1f, -10, 10, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(initVelPosAmplifier);

        NOTIFY(ImGui::DragFloat("Init Velocity Axis Amplifier", &header.initVelAxisAmplifier, 0.1f, -10, 10, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(initVelAxisAmplifier);

        ImGui::TextUnformatted("Rotation Speed");
        HELP(rotationSpeed);
        ImGui::Indent();
        NOTIFY(ImGui::SliderAngle("Min", &header.minRotation, 0, header.maxRotation * 180 / glm::pi<f32>()));
        NOTIFY(ImGui::SliderAngle("Max", &header.maxRotation, header.minRotation * 180 / glm::pi<f32>(), 360));
        ImGui::Unindent();

        ImGui::TextUnformatted("Variance");
        HELP(variance);
        ImGui::Indent();
        NOTIFY(ImGui::SliderFloat("Base Scale##variance", &header.variance.baseScale, 0, 1));
        NOTIFY(ImGui::SliderFloat("Particle Lifetime##variance", &header.variance.lifeTime, 0, 1));
        NOTIFY(ImGui::SliderFloat("Init Velocity##variance", &header.variance.initVel, 0, 1));
        ImGui::Unindent();

        NOTIFY(ImGui::SliderFloat("Air Resistance", &misc.airResistance, 0.75f, 1.25f));
        HELP(airResistance);

        NOTIFY(ImGui::SliderFloat("Loop Time", &misc.loopTime, frameTime, 8.5f, "%.4fs"));
        HELP(loopTime);

        u32 loops = (u32)glm::ceil(header.particleLifeTime / misc.loopTime);
        const u32 maxLoops = (u32)(header.particleLifeTime / frameTime);
        if (NOTIFY(ImGui::SliderInt("Loops", (int*)&loops, 1, maxLoops))) {
            misc.loopTime = header.particleLifeTime / (f32)loops;
        }
        HELP(loops);

        NOTIFY(ImGui::Checkbox("Randomize Looped Anim", &flags.randomizeLoopedAnim));
        HELP(randomizeLoopedAnim);

        NOTIFY(ImGui::SliderFloat("DBB Scale", &misc.dbbScale, -8.0f, 7.0f));
        HELP(dbbScale);

        if (ImGui::BeginCombo("Scale Anim Axis", getScaleAnimDir(misc.scaleAnimDir))) {
            for (const auto [val, name] : detail::g_scaleAnimDirNames) {
                if (NOTIFY(ImGui::Selectable(name, misc.scaleAnimDir == val))) {
                    misc.scaleAnimDir = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(scaleAnimDir);

        ImGui::TextUnformatted("Texture Tiling");
        HELP(textureTiling);
        ImGui::Indent();
        int tileCount = 1 << misc.textureTileCountS;
        NOTIFY(ImGui::SliderInt("S", &tileCount, 1, 8));
        misc.textureTileCountS = glm::log2(tileCount);

        tileCount = 1 << misc.textureTileCountT;
        NOTIFY(ImGui::SliderInt("T", &tileCount, 1, 8));
        misc.textureTileCountT = glm::log2(tileCount);
        ImGui::Unindent();

        NOTIFY(ImGui::Checkbox("DPol Face Emitter", &misc.dpolFaceEmitter));
        HELP(dpolFaceEmitter);

        NOTIFY(ImGui::Checkbox("Flip X", &misc.flipTextureS));
        HELP(flipTextureX);

        NOTIFY(ImGui::Checkbox("Flip Y", &misc.flipTextureT));
        HELP(flipTextureY);

        ImGui::TextUnformatted("Polygon Offset");
        HELP(polygonOffset);
        ImGui::Indent();
        NOTIFY(ImGui::SliderFloat("X", &header.polygonX, -2, 2));
        NOTIFY(ImGui::SliderFloat("Y", &header.polygonY, -2, 2));
        ImGui::Unindent();

        if (ImGui::BeginPopup("##texturePicker")) {
            for (int i = 0; i < textures.size(); ++i) {
                const auto& texture = textures[i];
                if (NOTIFY(ImGui::ImageButton((ImTextureID)texture.glTexture->getHandle(), { 32, 32 }))) {
                    header.misc.textureIndex = i;
                    ImGui::CloseCurrentPopup();
                }

                if (i % 4 != 3) {
                    ImGui::SameLine();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::TreePop();
    }
}

void Editor::renderBehaviorEditor(SPLResource& res) {
    LOCK_EDITOR();
    std::vector<std::shared_ptr<SPLBehavior>> toRemove;

    if (ImGui::Button("Add Behavior...")) {
        ImGui::OpenPopup("##addBehavior");
    }

    if (ImGui::BeginPopup("##addBehavior")) {
        if (NOTIFY(ImGui::MenuItem("Gravity", nullptr, false, !res.header.flags.hasGravityBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLGravityBehavior>(glm::vec3(0, 0, 0)));
            res.header.addBehavior(SPLBehaviorType::Gravity);
        }

        if (NOTIFY(ImGui::MenuItem("Random", nullptr, false, !res.header.flags.hasRandomBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLRandomBehavior>(glm::vec3(0, 0, 0), 1));
            res.header.addBehavior(SPLBehaviorType::Random);
        }

        if (NOTIFY(ImGui::MenuItem("Magnet", nullptr, false, !res.header.flags.hasMagnetBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLMagnetBehavior>(glm::vec3(0, 0, 0), 0));
            res.header.addBehavior(SPLBehaviorType::Magnet);
        }

        if (NOTIFY(ImGui::MenuItem("Spin", nullptr, false, !res.header.flags.hasSpinBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLSpinBehavior>(0, SPLSpinAxis::Y));
            res.header.addBehavior(SPLBehaviorType::Spin);
        }

        if (NOTIFY(ImGui::MenuItem("Collision Plane", nullptr, false, !res.header.flags.hasCollisionPlaneBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLCollisionPlaneBehavior>(0, 0, SPLCollisionType::Bounce));
            res.header.addBehavior(SPLBehaviorType::CollisionPlane);
        }

        if (NOTIFY(ImGui::MenuItem("Convergence", nullptr, false, !res.header.flags.hasConvergenceBehavior))) {
            res.behaviors.push_back(std::make_shared<SPLConvergenceBehavior>(glm::vec3(0, 0, 0), 0));
            res.header.addBehavior(SPLBehaviorType::Convergence);
        }

        ImGui::EndPopup();
    }

    for (const auto& bhv : res.behaviors) {
        ImGui::PushID(bhv.get());

        bool context = false;
        switch (bhv->type) {
        case SPLBehaviorType::Gravity:
            context = renderGravityBehaviorEditor(std::static_pointer_cast<SPLGravityBehavior>(bhv));
            break;
        case SPLBehaviorType::Random:
            context = renderRandomBehaviorEditor(std::static_pointer_cast<SPLRandomBehavior>(bhv));
            break;
        case SPLBehaviorType::Magnet:
            context = renderMagnetBehaviorEditor(std::static_pointer_cast<SPLMagnetBehavior>(bhv));
            break;
        case SPLBehaviorType::Spin:
            context = renderSpinBehaviorEditor(std::static_pointer_cast<SPLSpinBehavior>(bhv));
            break;
        case SPLBehaviorType::CollisionPlane:
            context = renderCollisionPlaneBehaviorEditor(std::static_pointer_cast<SPLCollisionPlaneBehavior>(bhv));
            break;
        case SPLBehaviorType::Convergence:
            context = renderConvergenceBehaviorEditor(std::static_pointer_cast<SPLConvergenceBehavior>(bhv));
            break;
        }

        if (context) {
            if (NOTIFY(ImGui::MenuItem("Delete"))) {
                toRemove.push_back(bhv);
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    for (const auto& r : toRemove) {
        std::erase(res.behaviors, r);
        res.header.removeBehavior(r->type);
    }
}

bool Editor::renderGravityBehaviorEditor(const std::shared_ptr<SPLGravityBehavior>& gravity) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##gravityEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Gravity");

    NOTIFY(ImGui::DragFloat3("Magnitude", glm::value_ptr(gravity->magnitude)));

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

bool Editor::renderRandomBehaviorEditor(const std::shared_ptr<SPLRandomBehavior>& random) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##randomEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Random");

    NOTIFY(ImGui::DragFloat3("Magnitude", glm::value_ptr(random->magnitude)));
    NOTIFY(ImGui::SliderFloat("Apply Interval", &random->applyInterval, 0, 5, "%.3fs", ImGuiSliderFlags_Logarithmic));

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

bool Editor::renderMagnetBehaviorEditor(const std::shared_ptr<SPLMagnetBehavior>& magnet) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##magnetEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Magnet");

    NOTIFY(ImGui::DragFloat3("Target", glm::value_ptr(magnet->target), 0.05f, -5.0f, 5.0f));
    NOTIFY(ImGui::SliderFloat("Force", &magnet->force, 0, 5, "%.3f", ImGuiSliderFlags_Logarithmic));

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

bool Editor::renderSpinBehaviorEditor(const std::shared_ptr<SPLSpinBehavior>& spin) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##spinEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Spin");

    NOTIFY(ImGui::SliderAngle("Angle", &spin->angle));
    ImGui::TextUnformatted("Axis");
    ImGui::Indent();
    NOTIFY(ImGui::RadioButton("X", (int*)&spin->axis, 0));
    NOTIFY(ImGui::RadioButton("Y", (int*)&spin->axis, 1));
    NOTIFY(ImGui::RadioButton("Z", (int*)&spin->axis, 2));
    ImGui::Unindent();

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

bool Editor::renderCollisionPlaneBehaviorEditor(const std::shared_ptr<SPLCollisionPlaneBehavior>& collisionPlane) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##collisionPlaneEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Collision Plane");

    NOTIFY(ImGui::DragFloat("Height", &collisionPlane->y, 0.05f));
    NOTIFY(ImGui::SliderFloat("Elasticity", &collisionPlane->elasticity, 0, 2, "%.3f", ImGuiSliderFlags_Logarithmic));
    ImGui::TextUnformatted("Collision Type");
    ImGui::Indent();
    NOTIFY(ImGui::RadioButton("Kill", (int*)&collisionPlane->collisionType, 0));
    NOTIFY(ImGui::RadioButton("Bounce", (int*)&collisionPlane->collisionType, 1));
    ImGui::Unindent();

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

bool Editor::renderConvergenceBehaviorEditor(const std::shared_ptr<SPLConvergenceBehavior>& convergence) {
    LOCK_EDITOR();
    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }
    ImGui::BeginChild("##convergenceEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted("Convergence");

    NOTIFY(ImGui::DragFloat3("Target", glm::value_ptr(convergence->target), 0.05f, -5.0f, 5.0f));
    NOTIFY(ImGui::SliderFloat("Force", &convergence->force, -5, 5, "%.3f", ImGuiSliderFlags_Logarithmic));

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    return ImGui::BeginPopupContextItem("##behaviorContext");
}

void Editor::renderAnimationEditor(SPLResource& res) {
    if (m_activeEditor.expired()) {
        return;
    }

    LOCK_EDITOR();

    if (ImGui::Button("Add Animation...")) {
        ImGui::OpenPopup("##addAnimation");
    }

    if (ImGui::BeginPopup("##addAnimation")) {
        if (NOTIFY(ImGui::MenuItem("Scale", nullptr, false, !res.header.flags.hasScaleAnim))) {
            res.addScaleAnim(SPLScaleAnim::createDefault());
            ImGui::CloseCurrentPopup();
        }

        if (NOTIFY(ImGui::MenuItem("Color", nullptr, false, !res.header.flags.hasColorAnim))) {
            res.addColorAnim(SPLColorAnim::createDefault());
            ImGui::CloseCurrentPopup();
        }

        if (NOTIFY(ImGui::MenuItem("Alpha", nullptr, false, !res.header.flags.hasAlphaAnim))) {
            res.addAlphaAnim(SPLAlphaAnim::createDefault());
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (res.scaleAnim) {
        if (renderScaleAnimEditor(*res.scaleAnim)) {
            res.removeScaleAnim();
        }
    }

    if (res.colorAnim) {
        if (renderColorAnimEditor(res, *res.colorAnim)) {
            res.removeColorAnim();
        }
    }

    if (res.alphaAnim) {
        if (renderAlphaAnimEditor(*res.alphaAnim)) {
            res.removeAlphaAnim();
        }
    }
}

bool Editor::renderScaleAnimEditor(SPLScaleAnim& res) {
    LOCK_EDITOR();

    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }

    if (!ImGui::CollapsingHeader("Scale Animation")) {
        return false;
    }

    ImGui::BeginChild("##scaleAnimEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

    NOTIFY(ImGui::SliderFloat("Start Scale", &res.start, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
    NOTIFY(ImGui::SliderFloat("Mid Scale", &res.mid, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
    NOTIFY(ImGui::SliderFloat("End Scale", &res.end, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic));
    constexpr u8 min = 0;
    constexpr u8 max = 255;
    NOTIFY(ImGui::SliderScalar("In", ImGuiDataType_U8, &res.curve.in, &min, &max, "%u"));
    NOTIFY(ImGui::SliderScalar("Out", ImGuiDataType_U8, &res.curve.out, &min, &max, "%u"));
    NOTIFY(ImGui::Checkbox("Loop", &res.flags.loop));

    res.plot(m_xAnimBuffer, m_yAnimBuffer);
    if (ImPlot::BeginPlot("##scaleAnimPlot", {-1, 0}, ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Scale", m_xAnimBuffer.data(), m_yAnimBuffer.data(), m_xAnimBuffer.size());
        ImPlot::EndPlot();
    }

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    if (ImGui::BeginPopupContextItem("##scaleAnimContext")) {
        if (ImGui::MenuItem("Delete")) {
            ImGui::CloseCurrentPopup();
            return true;
        }

        ImGui::EndPopup();
    }

    return false;
}

bool Editor::renderColorAnimEditor(const SPLResource& mainRes, SPLColorAnim& res) {
    LOCK_EDITOR();

    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }

    if (!ImGui::CollapsingHeader("Color Animation")) {
        return false;
    }

    ImGui::BeginChild("##colorAnimEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

    NOTIFY(ImGui::ColorEdit3("Start Color", glm::value_ptr(res.start)));
    NOTIFY(ImGui::ColorEdit3("End Color", glm::value_ptr(res.end)));
    constexpr u8 min = 0;
    constexpr u8 max = 255;
    NOTIFY(ImGui::SliderScalar("In", ImGuiDataType_U8, &res.curve.in, &min, &max, "%u"));
    NOTIFY(ImGui::SliderScalar("Peak", ImGuiDataType_U8, &res.curve.peak, &min, &max, "%u"));
    NOTIFY(ImGui::SliderScalar("Out", ImGuiDataType_U8, &res.curve.out, &min, &max, "%u"));

    NOTIFY(ImGui::Checkbox("Loop", &res.flags.loop));
    NOTIFY(ImGui::Checkbox("Interpolate", &res.flags.interpolate));
    NOTIFY(ImGui::Checkbox("Random Start Color", &res.flags.randomStartColor));
    
    const auto drawList = ImGui::GetWindowDrawList();
    const auto startPos = ImGui::GetCursorScreenPos();
    const auto maxWidth = ImGui::GetContentRegionAvail().x;
    auto pos = startPos;

    constexpr auto toImColor = [](const glm::vec3& color) {
        return IM_COL32(
            (u8)(color.r * 255),
            (u8)(color.g * 255),
            (u8)(color.b * 255),
            255
        );
    };

    const auto in = res.curve.getIn();
    const auto peak = res.curve.getPeak();
    const auto out = res.curve.getOut();

    const auto startCol = toImColor(res.start);
    const auto peakCol = toImColor(mainRes.header.color);
    const auto endCol = toImColor(res.end);

    if (in > 0.0f) {
        const auto endPos = ImVec2(pos.x + in * maxWidth, pos.y + 20);
        drawList->AddRectFilled(pos, endPos, startCol);
        pos.x = endPos.x;
    }

    if (res.flags.interpolate) {
        const auto endPos = ImVec2(pos.x + (peak - in) * maxWidth, pos.y + 20);
        drawList->AddRectFilledMultiColor(
            pos,
            endPos,
            startCol,
            peakCol,
            peakCol,
            startCol
        );
        pos.x = endPos.x;
    } else {
        const auto endPos = ImVec2(pos.x + (peak - in) * maxWidth, pos.y + 20);
        drawList->AddRectFilled(pos, endPos, toImColor(mainRes.header.color));
        pos.x = endPos.x;
    }

    if (res.flags.interpolate) {
        const auto endPos = ImVec2(pos.x + (out - peak) * maxWidth, pos.y + 20);
        drawList->AddRectFilledMultiColor(
            pos,
            endPos,
            peakCol,
            endCol,
            endCol,
            peakCol
        );
        pos.x = endPos.x;
    } else {
        const auto endPos = ImVec2(pos.x + (out - peak) * maxWidth, pos.y + 20);
        drawList->AddRectFilled(pos, endPos, endCol);
        pos.x = endPos.x;
    }

    if (out < 1.0f) {
        const auto endPos = ImVec2(pos.x + (1.0f - out) * maxWidth, pos.y + 20);
        drawList->AddRectFilled(pos, endPos, endCol);
    }

    ImGui::Dummy({ maxWidth, 20 });

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    if (ImGui::BeginPopupContextItem("##colorAnimContext")) {
        if (ImGui::MenuItem("Delete")) {
            ImGui::CloseCurrentPopup();
            return true;
        }

        ImGui::EndPopup();
    }

    return false;
}

bool Editor::renderAlphaAnimEditor(SPLAlphaAnim& res) {
    LOCK_EDITOR();

    static bool hovered = false;
    if (hovered) {
        ImGui::PushStyleColor(ImGuiCol_Border, s_hoverAccentColor);
    }

    if (!ImGui::CollapsingHeader("Alpha Animation")) {
        return false;
    }

    ImGui::BeginChild("##alphaAnimEditor", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);

    NOTIFY(ImGui::SliderFloat("Start Alpha", &res.alpha.start, 0, 1));
    NOTIFY(ImGui::SliderFloat("Mid Alpha", &res.alpha.mid, 0, 1));
    NOTIFY(ImGui::SliderFloat("End Alpha", &res.alpha.end, 0, 1));

    res.alpha.start = (f32)((u8)(res.alpha.start * 31.0f)) / 31.0f;
    res.alpha.mid = (f32)((u8)(res.alpha.mid * 31.0f)) / 31.0f;
    res.alpha.end = (f32)((u8)(res.alpha.end * 31.0f)) / 31.0f;

    const auto minAlpha = std::min({ res.alpha.start, res.alpha.mid, res.alpha.end });
    const auto maxAlpha = std::max({ res.alpha.start, res.alpha.mid, res.alpha.end });

    constexpr u8 min = 0;
    constexpr u8 max = 255;
    NOTIFY(ImGui::SliderScalar("In", ImGuiDataType_U8, &res.curve.in, &min, &max, "%u"));
    NOTIFY(ImGui::SliderScalar("Out", ImGuiDataType_U8, &res.curve.out, &min, &max, "%u"));
    NOTIFY(ImGui::SliderFloat("Random Range", &res.flags.randomRange, 0, 1));
    NOTIFY(ImGui::Checkbox("Loop", &res.flags.loop));

    res.plot(m_xAnimBuffer, m_yAnimBuffer);

    if (ImPlot::BeginPlot("##alphaAnimPlot", {-1, 0}, ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1);
        ImPlot::SetupAxisLimits(ImAxis_Y1, minAlpha, maxAlpha);
        ImPlot::PlotLine("Alpha", m_xAnimBuffer.data(), m_yAnimBuffer.data(), m_xAnimBuffer.size());
        ImPlot::EndPlot();
    }

    ImGui::EndChild();

    if (hovered) {
        ImGui::PopStyleColor();
    }

    hovered = ImGui::IsItemHovered();
    if (ImGui::BeginPopupContextItem("##alphaAnimContext")) {
        if (ImGui::MenuItem("Delete")) {
            ImGui::CloseCurrentPopup();
            return true;
        }

        ImGui::EndPopup();
    }

    return false;
}

bool Editor::renderTexAnimEditor(SPLTexAnim& res) {
    return false;
}

void Editor::renderChildrenEditor(SPLResource& res) {
    if (m_activeEditor.expired()) {
        return;
    }

    LOCK_EDITOR();

    if (!res.childResource) {
        ImGui::TextUnformatted("This resource does not have an associated child resource.");
        if (ImGui::Button("Add Child Resource")) {
            res.header.flags.hasChildResource = true;
            res.childResource = SPLChildResource{
                .flags = {
                    .usesBehaviors = false,
                    .hasScaleAnim = false,
                    .hasAlphaAnim = false,
                    .rotationType = SPLChildRotationType::None,
                    .followEmitter = false,
                    .useChildColor = false,
                    .drawType = SPLDrawType::Billboard,
                    .polygonRotAxis = SPLPolygonRotAxis::Y,
                    .polygonReferencePlane = 0
                },
                .randomInitVelMag = 0.0f,
                .endScale = 1.0f,
                .lifeTime = 1.0f / SPLArchive::SPL_FRAMES_PER_SECOND,
                .velocityRatio = 1.0f,
                .scaleRatio = 1.0f,
                .color = {},
                .misc = {
                    .emissionCount = 0,
                    .emissionDelay = 0,
                    .emissionInterval = 1.0f / SPLArchive::SPL_FRAMES_PER_SECOND,
                    .texture = 0,
                    .textureTileCountS = 1,
                    .textureTileCountT = 1,
                    .flipTextureS = false,
                    .flipTextureT = false,
                    .dpolFaceEmitter = false
                }
            };
        }

        return;
    }

    auto& child = res.childResource.value();
    constexpr f32 frameTime = 1.0f / (f32)SPLArchive::SPL_FRAMES_PER_SECOND;
    constexpr auto helpPopup = [](std::string_view text) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.data());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    };

    bool open = ImGui::TreeNodeEx("##parentSettings", ImGuiTreeNodeFlags_SpanAvailWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
    ImGui::SeparatorText("Parent Settings");
    if (open) {
        NOTIFY(ImGui::DragInt("Emission Amount", (int*)&child.misc.emissionCount, 1, 0, 20));
        HELP(emissionCount);

        NOTIFY(ImGui::SliderFloat("Emission Delay", &child.misc.emissionDelay, 0, 1));
        HELP(childEmissionDelay);

        NOTIFY(ImGui::SliderFloat("Emission Interval", &child.misc.emissionInterval, frameTime, 8.5f, "%.4fs"));
        HELP(childEmissionInterval);

        u32 emissions = (u32)glm::ceil(res.header.particleLifeTime / child.misc.emissionInterval);
        const u32 maxEmissions = (u32)(res.header.particleLifeTime / frameTime);
        if (NOTIFY(ImGui::SliderInt("Emissions", (int*)&emissions, 1, maxEmissions))) {
            child.misc.emissionInterval = res.header.particleLifeTime / (f32)emissions;
        }
        HELP(childEmissions);

        ImGui::TreePop();
    }

    open = ImGui::TreeNodeEx("##childSettings", ImGuiTreeNodeFlags_SpanAvailWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
    ImGui::SeparatorText("Child Settings");
    if (open) {
        auto& flags = child.flags;
        auto& misc = child.misc;

        if (ImGui::BeginCombo("Draw Type", getDrawType(flags.drawType))) {
            for (const auto [val, name] : detail::g_drawTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.drawType == val))) {
                    flags.drawType = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(drawType);

        const auto& textures = LOCKED_EDITOR()->getArchive().getTextures();
        if (ImGui::ImageButton((ImTextureID)textures[misc.texture].glTexture->getHandle(), { 32, 32 })) {
            ImGui::OpenPopup("##childTexturePicker");
        }
        ImGui::SameLine();
        ImGui::Text("Texture");
        HELP(childTexture);

        if (ImGui::BeginCombo("Child Rotation", getChildRotType(flags.rotationType))) {
            for (const auto [val, name] : detail::g_childRotTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.rotationType == val))) {
                    flags.rotationType = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(childRotation);

        ImGui::BeginDisabled((u8)flags.drawType < (u8)SPLDrawType::Polygon);

        if (ImGui::BeginCombo("Polygon Rotation Axis", getPolygonRotAxis(flags.polygonRotAxis))) {
            for (const auto [val, name] : detail::g_polygonRotAxisNames) {
                if (NOTIFY(ImGui::Selectable(name, flags.polygonRotAxis == val))) {
                    flags.polygonRotAxis = val;
                }
            }

            ImGui::EndCombo();
        }
        HELP(polygonRotAxis);

        ImGui::TextUnformatted("Polygon Reference Plane");
        HELP(polygonReferencePlane);
        ImGui::Indent();
        NOTIFY(ImGui::RadioButton("XY", &flags.polygonReferencePlane, 0));
        NOTIFY(ImGui::RadioButton("XZ", &flags.polygonReferencePlane, 1));
        ImGui::Unindent();

        ImGui::EndDisabled();

        NOTIFY(ImGui::Checkbox("Uses Behaviors", &flags.usesBehaviors));
        HELP(usesBehaviors);

        NOTIFY(ImGui::Checkbox("Follow Emitter", &flags.followEmitter));
        HELP(followEmitter);

        NOTIFY(ImGui::SliderFloat("Lifetime", &child.lifeTime, frameTime, 60, "%.4fs", ImGuiSliderFlags_Logarithmic));
        HELP(particleLifeTime);

        NOTIFY(ImGui::SliderFloat("Initial Velocity Random", &child.randomInitVelMag, -3, 3, "%.3f", ImGuiSliderFlags_Logarithmic));
        HELP(randomInitVelMag);

        NOTIFY(ImGui::SliderFloat("Velocity Ratio", &child.velocityRatio, 0, 1));
        HELP(velocityRatio);

        NOTIFY(ImGui::SliderFloat("Scale Ratio", &child.scaleRatio, 0, 1));
        HELP(scaleRatio);

        NOTIFY(ImGui::ColorEdit3("Color", glm::value_ptr(child.color)));
        HELP(color);

        NOTIFY(ImGui::Checkbox("Use Color", &flags.useChildColor));
        HELP(useChildColor);

        ImGui::TextUnformatted("Texture Tiling");
        HELP(textureTiling);
        ImGui::Indent();
        int tileCount = 1 << misc.textureTileCountS;
        NOTIFY(ImGui::SliderInt("S", &tileCount, 1, 8));
        misc.textureTileCountS = glm::log2(tileCount);

        tileCount = 1 << misc.textureTileCountT;
        NOTIFY(ImGui::SliderInt("T", &tileCount, 1, 8));
        misc.textureTileCountT = glm::log2(tileCount);
        ImGui::Unindent();

        NOTIFY(ImGui::Checkbox("DPol Face Emitter", &misc.dpolFaceEmitter));
        HELP(dpolFaceEmitter);

        NOTIFY(ImGui::Checkbox("Flip X", &misc.flipTextureS));
        HELP(flipTextureX);

        NOTIFY(ImGui::Checkbox("Flip Y", &misc.flipTextureT));
        HELP(flipTextureY);

        NOTIFY(ImGui::Checkbox("Scale Animation", &flags.hasScaleAnim));
        HELP(hasScaleAnim);
        if (flags.hasScaleAnim) {
            NOTIFY(ImGui::SliderFloat("End Scale", &child.endScale, 0, 5, "%.3f", ImGuiSliderFlags_Logarithmic));
            HELP(endScale);
        }

        NOTIFY(ImGui::Checkbox("Fade Out", &flags.hasAlphaAnim));
        HELP(hasAlphaAnim);

        if (ImGui::BeginPopup("##childTexturePicker")) {
            for (int i = 0; i < textures.size(); ++i) {
                const auto& texture = textures[i];
                if (NOTIFY(ImGui::ImageButton((ImTextureID)texture.glTexture->getHandle(), { 32, 32 }))) {
                    child.misc.texture = i;
                    ImGui::CloseCurrentPopup();
                }

                if ((i + 1) % 4 != 0) {
                    ImGui::SameLine();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::TreePop();
    }
}

void Editor::openTempTexture(std::string_view path) {
    const auto tempTex = new TempTexture;
    m_tempTexture = tempTex;

    tempTex->path = path;
    tempTex->data = stbi_load(path.data(), &tempTex->width, &tempTex->height, &tempTex->channels, 4);
    if (!tempTex->data) {
        return;
    }

    tempTex->texture = std::make_unique<GLTexture>(tempTex->width, tempTex->height);
    tempTex->quantized = new u8[tempTex->width * tempTex->height * 4];

    tempTex->preference = TextureConversionPreference::ColorDepth;
    tempTex->suggestedSpec = SPLTexture::suggestSpecification(tempTex->width, tempTex->height, tempTex->channels, tempTex->data, tempTex->preference);

    if (tempTex->suggestedSpec.requiresColorCompression) {
        quantizeTexture(tempTex->data, tempTex->width, tempTex->height, tempTex->suggestedSpec, tempTex->quantized);
        tempTex->texture->update(tempTex->quantized);
    } else {
        tempTex->texture->update(tempTex->data);
    }
}

void Editor::discardTempTexture() {
    stbi_image_free(m_tempTexture->data);
    delete m_tempTexture->quantized;
    delete m_tempTexture;
}

void Editor::importTempTexture() {
    if (!m_tempTexture) {
        return;
    }

    auto& activeEditor = g_projectManager->getActiveEditor();
    auto& archive = activeEditor->getArchive();
    auto& textures = archive.getTextures();
    auto& texture = textures.emplace_back();
    texture.glTexture = std::move(m_tempTexture->texture);
    texture.width = m_tempTexture->width;
    texture.height = m_tempTexture->height;
    texture.param = {
        .format = m_tempTexture->suggestedSpec.format,
        .s = 1,
        .t = 1,
        .repeat = TextureRepeat::None,
        .flip = TextureFlip::None,
        .palColor0Transparent = false,
        .useSharedTexture = false,
        .sharedTexID = 0xFF
    };

    auto& textureData = archive.getTextureData().emplace_back();
    auto& paletteData = archive.getPaletteData().emplace_back();

    palettizeTexture(
        m_tempTexture->quantized,
        m_tempTexture->width,
        m_tempTexture->height,
        m_tempTexture->suggestedSpec,
        textureData,
        paletteData
    );

    texture.textureData = textureData;
    texture.paletteData = paletteData;

    discardTempTexture();

    activeEditor->getParticleSystem().getRenderer().setTextures(textures);
}

bool Editor::palettizeTexture(const u8* data, s32 width, s32 height, const TextureImportSpecification& spec, std::vector<u8>& outData, std::vector<u8>& outPalette) {
    return SPLTexture::convertFromRGBA8888(data, width, height, spec.format, outData, outPalette);
}

void Editor::quantizeTexture(const u8* data, s32 width, s32 height, const TextureImportSpecification& spec, u8* out) {
    liq_attr* attr = liq_attr_create();
    liq_set_max_colors(attr, spec.getMaxColors());
    
    liq_image* image = liq_image_create_rgba(attr, data, width, height, 0);
    
    liq_error err;
    liq_result* result = nullptr;
    if ((err = liq_image_quantize(image, attr, &result)) != LIQ_OK) {
        spdlog::error("Failed to quantize image: {}", (int)err);
        return;
    }

    std::vector<u8> quantized(width * height);
    if ((err = liq_write_remapped_image(result, image, quantized.data(), quantized.size())) != LIQ_OK) {
        spdlog::error("Failed to write quantized image: {}", (int)err);
        return;
    }

    const auto palette = liq_get_palette(result);
    if (palette->count > spec.getMaxColors()) {
        spdlog::error("Too many colors in resulting palette");
        return;
    }

    auto palcopy = *palette;
    std::span<liq_color> colorspan(palcopy.entries, palcopy.count);

    // Further quantize alpha values if necessary
    if (spec.requiresAlphaCompression) {
        const auto maxAlphas = spec.getMaxAlphas();
        const auto [min, max] = spec.getAlphaRange();

        const auto transparent = std::ranges::find_if(colorspan, [](liq_color color) { return color.a == 0; });

        switch (spec.format) {
        case TextureFormat::None: break;
        case TextureFormat::A3I5:
        case TextureFormat::A5I3:
            std::ranges::for_each(colorspan, [min, max](auto& color) {
                const auto mapped = (u8)(((float)color.a / 255.0f) * (max - min)) + min;
                color.a = (u8)(((float)(mapped - min) / (max - min)) * 255.0f);
            });
            break;

        case TextureFormat::Palette4:
        case TextureFormat::Palette16:
        case TextureFormat::Palette256:
        case TextureFormat::Direct:
            if (spec.needsAlpha() || spec.format == TextureFormat::Direct) {
                std::ranges::for_each(colorspan, [](auto& color) {
                    if (color.a < 128) {
                        color.a = 0;
                    } else {
                        color.a = 255;
                    }
                });
            }
            break;
        default: break;
        }
    }

    const auto colors = (u32*)palcopy.entries;
    std::span<u32> remapped((u32*)out, width * height);
    for (auto [index, pixel] : std::views::zip(quantized, remapped)) {
        pixel = colors[index];
    }

    liq_result_destroy(result);
    liq_image_destroy(image);
    liq_attr_destroy(attr);
}
