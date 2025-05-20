#include "extensions.h"

#include <imgui_internal.h>


bool ImGui::GradientButton(const char* label, const ImVec2& size, ImU32 textColor, ImU32 bgColor1, ImU32 bgColor2) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 item_size = CalcItemSize(size, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + item_size);
    ItemSize(item_size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    ImGuiButtonFlags flags = ImGuiButtonFlags_None;
    if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
        flags |= ImGuiButtonFlags_Repeat;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const bool is_gradient = bgColor1 != bgColor2;
    if (held || hovered)
    {
        // Modify colors (ultimately this can be prebaked in the style)
        float h_increase = (held && hovered) ? 0.02f : 0.02f;
        float v_increase = (held && hovered) ? 0.20f : 0.07f;

        ImVec4 bg1f = ColorConvertU32ToFloat4(bgColor1);
        ColorConvertRGBtoHSV(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
        bg1f.x = ImMin(bg1f.x + h_increase, 1.0f);
        bg1f.z = ImMin(bg1f.z + v_increase, 1.0f);
        ColorConvertHSVtoRGB(bg1f.x, bg1f.y, bg1f.z, bg1f.x, bg1f.y, bg1f.z);
        bgColor1 = GetColorU32(bg1f);
        if (is_gradient)
        {
            ImVec4 bg2f = ColorConvertU32ToFloat4(bgColor2);
            ColorConvertRGBtoHSV(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
            bg2f.z = ImMin(bg2f.z + h_increase, 1.0f);
            bg2f.z = ImMin(bg2f.z + v_increase, 1.0f);
            ColorConvertHSVtoRGB(bg2f.x, bg2f.y, bg2f.z, bg2f.x, bg2f.y, bg2f.z);
            bgColor2 = GetColorU32(bg2f);
        }
        else
        {
            bgColor2 = bgColor1;
        }
    }
    RenderNavHighlight(bb, id);

#if 0
    window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, bg_color_1, bg_color_1, bg_color_2, bg_color_2);
    if (g.Style.FrameBorderSize > 0.0f)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), 0.0f, 0, g.Style.FrameBorderSize);
#endif

    int vert_start_idx = window->DrawList->VtxBuffer.Size;
    window->DrawList->AddRectFilled(bb.Min, bb.Max, bgColor1, g.Style.FrameRounding);
    int vert_end_idx = window->DrawList->VtxBuffer.Size;
    if (is_gradient)
        ShadeVertsLinearColorGradientKeepAlpha(window->DrawList, vert_start_idx, vert_end_idx, bb.Min, bb.GetBL(), bgColor1, bgColor2);
    if (g.Style.FrameBorderSize > 0.0f)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), g.Style.FrameRounding, 0, g.Style.FrameBorderSize);

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");
    PushStyleColor(ImGuiCol_Text, textColor);
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
    PopStyleColor();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool ImGui::RedButton(const char* label, const ImVec2& size) {
    return GradientButton(label, size, IM_COL32(213, 213, 213, 255), IM_COL32(137, 32, 37, 255), IM_COL32(85, 20, 23, 255));
}

bool ImGui::GreenButton(const char* label, const ImVec2& size) {
    return GradientButton(label, size, IM_COL32(213, 213, 213, 255), IM_COL32(35, 134, 54, 255), IM_COL32(21, 83, 33, 255));
}

bool ImGui::BlueButton(const char* label, const ImVec2& size) {
    return GradientButton(label, size, IM_COL32(213, 213, 213, 255), IM_COL32(34, 80, 104, 255), IM_COL32(25, 48, 60, 255));
}

bool ImGui::GreyButton(const char* label, const ImVec2& size) {
    return GradientButton(label, size, IM_COL32(0, 0, 0, 255), IM_COL32(174, 174, 174, 255), IM_COL32(107, 107, 107, 255));
}

bool ImGui::MenuItemIcon(const char* icon, const char* label, const char* shortcut, bool selected, bool enabled) {
    return MenuItemEx(label, icon, shortcut, selected, enabled);
}

bool ImGui::MenuItemIcon(const char* icon, const char* label, const char* shortcut, bool* selected, bool enabled) {
    const bool result = MenuItemEx(label, icon, shortcut, selected ? *selected : false, enabled);
    if (result && selected) {
        *selected = !*selected;
    }
    
    return result;
}

bool ImGui::PaddedTreeNode(const char* label, const ImVec2& padding, ImGuiTreeNodeFlags flags) {
    PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    const bool open = TreeNodeEx(label, flags | ImGuiTreeNodeFlags_FramePadding);
    PopStyleVar();
    return open;
}
