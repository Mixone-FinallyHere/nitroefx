#include "editor.h"
#include "project_manager.h"
#include "spl/enum_names.h"

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_internal.h>


void Editor::render() {
    const auto& instances = g_projectManager->getOpenEditors();

    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar
        | ImGuiDockNodeFlags_NoDockingOverCentralNode
        | ImGuiDockNodeFlags_NoUndocking;
    ImGui::SetNextWindowClass(&windowClass);

    ImGui::Begin("Work Area##Editor", nullptr,
        ImGuiWindowFlags_NoMove |
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

    if (m_picker_open) {
        renderResourcePicker();
    }

    if (m_editor_open) {
        renderResourceEditor();
    }
}

void Editor::renderParticles() {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->renderParticles();
}

void Editor::openPicker() {
    m_picker_open = true;
}

void Editor::openEditor() {
    m_editor_open = true;
}

void Editor::updateParticles(float deltaTime) {
    const auto& editor = g_projectManager->getActiveEditor();
    if (!editor) {
        return;
    }

    editor->updateParticles(deltaTime * m_timeScale);
}

void Editor::renderResourcePicker() {
    if (ImGui::Begin("Resource Picker##Editor", &m_picker_open)) {

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

            for (int i = 0; i < resources.size(); ++i) {
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
    }

    ImGui::End();
}

void Editor::renderResourceEditor() {
    if (ImGui::Begin("Resource Editor##Editor", &m_editor_open)) {
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
            static bool looped = false;
            auto& resource = resources[m_selectedResources[id]];
            const auto& texture = textures[resource.header.misc.textureIndex];

            if (ImGui::Button("Play Selected Emitter")) {
                editor->getParticleSystem().addEmitter(resource, looped);
            }

            ImGui::SameLine();
            ImGui::Checkbox("Spawn Looped Emitter", &looped);

            if (ImGui::Button("Kill Emitters")) {
                editor->getParticleSystem().killAllEmitters();
            }

            if (ImGui::CollapsingHeader("General")) {
                renderHeaderEditor(resource.header);
            }
        }
    }

    ImGui::End();

    m_activeEditor.reset();
}

void Editor::renderHeaderEditor(SPLResourceHeader& header) {
#define NOTIFY(action) m_activeEditor.lock()->valueChanged(action)

    if (m_activeEditor.expired()) {
        return;
    }

    constexpr auto help = [](std::string_view text) {
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

    if (ImGui::TreeNode("Attributes")) {
        if (ImGui::BeginCombo("Emission Type", getEmissionType(header.flags.emissionType))) {
            for (const auto [val, name] : detail::g_emissionTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, header.flags.emissionType == val))) {
                    header.flags.emissionType = val;
                }
            }

            ImGui::EndCombo();
        }
        help("The shape in which particles are emitted");

        if (ImGui::BeginCombo("Draw Type", getDrawType(header.flags.drawType))) {
            for (const auto [val, name] : detail::g_drawTypeNames) {
                if (NOTIFY(ImGui::Selectable(name, header.flags.drawType == val))) {
                    header.flags.drawType = val;
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Circle Axis", getEmissionAxis(header.flags.emissionAxis))) {
            for (const auto [val, name] : detail::g_emissionAxisNames) {
                if (NOTIFY(ImGui::Selectable(name, header.flags.emissionAxis == val))) {
                    header.flags.emissionAxis = val;
                }
            }

            ImGui::EndCombo();
        }
        help("For oriented emission shapes, this gives the axis of these shapes");

        NOTIFY(ImGui::Checkbox("Rotate", &header.flags.hasRotation));
        help("Whether particles should rotate");

        NOTIFY(ImGui::Checkbox("Random Init Angle", &header.flags.randomInitAngle));
        help("Whether particles should have a randomized initial angle");



        ImGui::TreePop();
    }
}
