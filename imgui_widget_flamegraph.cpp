// The MIT License(MIT)
//
// Copyright(c) 2019 Sandy Carter
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <imgui_widget_flamegraph.h>

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include <string>
#include <vector>

void ImGuiWidgetFlameGraph::PlotInfo(const char* label, void (*values_getter)(float* start, float* end, ImU8* level, const char** caption, const void* data, int idx), const void* data, int values_count, int values_offset)
{
    std::vector<float> values;
    values.reserve(values_count);
    float total = 0.0f;


    float stageStart, stageEnd;
    ImU8 depth;
    const char* caption;

    for (int i = values_offset; i < values_count; ++i)
    {
        
       
        values_getter(&stageStart, &stageEnd, &depth, &caption, data, i);
        total += stageEnd - stageStart;
    }
    const float sizeY = 15.0f;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGui::Text("%s", label);
    for (int i = values_offset; i < values_count; ++i)
    {
        values_getter(&stageStart, &stageEnd, &depth, &caption, data, i);
        const auto val = stageEnd - stageStart;
        const float perc = 100.0f * (val / total);
        auto pos = window->DC.CursorPos;

        pos.y += ImGui::GetTextLineHeight() - sizeY;
        std::string prestr = std::string(depth*2, ' ') + "> ";
        ImGui::Text("%s%s: %3.3f %2.0f%%", prestr.c_str(), caption, val , perc);

        ImVec2 graph_size(perc*10.0f, sizeY);
        const ImRect frame_bb(pos, pos + graph_size);
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    }
}


void ImGuiWidgetFlameGraph::PlotFlame(const char* label, void (*values_getter)(float* start, float* end, ImU8* level, const char** caption, const void* data, int idx), const void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    if (values_count == 0)
        return;
    // Find the maximum depth
    ImU8 maxDepth = 0;
    for (int i = values_offset; i < values_count; ++i)
    {
        ImU8 depth;
        values_getter(nullptr, nullptr, &depth, nullptr, data, i);
        maxDepth = ImMax(maxDepth, depth);
    }

    const auto blockHeight = 10+ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    if (graph_size.x == 0.0f)
        graph_size.x = ImGui::CalcItemWidth();

    if (graph_size.y == 0.0f)
        graph_size.y = label_size.y + (style.FramePadding.y * 3) + blockHeight * (maxDepth + 1);

    auto pos = window->DC.CursorPos;
    const ImRect frame_bb(pos, pos + graph_size);

    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
   const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));

    ImGui::ItemSize(total_bb, style.FramePadding.y);

    if (!ImGui::ItemAdd(total_bb, 0, &frame_bb))
        return;

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX)
    {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = values_offset; i < values_count; i++)
        {
            float v_start, v_end;
            values_getter(&v_start, &v_end, nullptr, nullptr, data, i);
            if (v_start == v_start) // Check non-NaN values
                v_min = ImMin(v_min, v_start);
       
            if (v_end == v_end) // Check non-NaN values
                v_max = ImMax(v_max, v_end);
        
        }

        if (scale_min == FLT_MAX)
            scale_min = v_min;

        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    bool any_hovered = false;
    if (values_count - values_offset >= 1)
    {
        const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x77FFFFFF;
        const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x77FFFFFF;
        const ImU32 col_outline_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x7FFFFFFF;
        const ImU32 col_outline_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x7FFFFFFF;

      for (int i = values_offset; i < values_count; ++i)
        {
            float stageStart, stageEnd;
            ImU8 depth;
            const char* caption;

            values_getter(&stageStart, &stageEnd, &depth, &caption, data, i);

            const auto duration = scale_max - scale_min;
            if (duration == 0)
            {
                return;
            }

            if (stageStart > stageEnd)
                return;

            const float start = stageStart - scale_min;
            const float end = stageEnd - scale_min;

            const double startX = start / static_cast<double>(duration);
            const double endX = end / static_cast<double>(duration);

            const double width = inner_bb.Max.x - inner_bb.Min.x;
           
            const double height = blockHeight * (maxDepth - depth + 1) - style.FramePadding.y;

            auto pos0 = inner_bb.Min + ImVec2((float)(startX * width), (float)height);
            auto pos1 = inner_bb.Min + ImVec2((float)(endX * width), (float)(height + blockHeight));

            bool v_hovered = false;
            if (ImGui::IsMouseHoveringRect(pos0, pos1))
            {
                ImGui::SetTooltip("%s: %8.4g", caption, stageEnd - stageStart);
                v_hovered = true;
                any_hovered = v_hovered;
            }
            if (pos1.x-pos0.x > width)
                return;
            window->DrawList->AddRectFilled(pos0, pos1, v_hovered ? col_hovered : col_base);
            window->DrawList->AddRect(pos0, pos1, v_hovered ? col_outline_hovered : col_outline_base);
            auto textSize = ImGui::CalcTextSize(caption);
            auto boxSize = (pos1 - pos0);
            auto textOffset = ImVec2(0.0f, 0.0f);
            if (textSize.x < boxSize.x)
            {
                textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
                ImGui::RenderText(pos0 + textOffset, caption);
            }
        }

        // Text overlay
        if (overlay_text)
            ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, nullptr, nullptr, ImVec2(0.5f, 0.0f));

        if (label_size.x > 0.0f)
            ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
    }


    if (!any_hovered && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Total: %8.4g", scale_max - scale_min);
    }
}
