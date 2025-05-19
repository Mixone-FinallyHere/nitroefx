#pragma once

#include <imgui.h>


namespace ImGui {

    bool GradientButton(const char* label, const ImVec2& size, ImU32 textColor, ImU32 bgColor, ImU32 bgColor2);
    bool GreenButton(const char* label, const ImVec2& size = { 0.0f, 0.0f });
    bool RedButton(const char* label, const ImVec2& size = { 0.0f, 0.0f });

    bool MenuItemIcon(const char* icon, const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
}
