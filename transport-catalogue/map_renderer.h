#pragma once
#include "domain.h"
#include "svg.h"
#include <vector>
#include <utility>
#include <map>
#include <deque>

namespace transport
{
    struct RenderSettings
    {
        double width = 0.0, height = 0.0, padding = 0.0, line_width = 0.0, stop_radius = 0.0, underlayer_width = 0.0;
        int bus_label_font_size = 0, stop_label_font_size = 0;
        std::pair<double, double> bus_label_offset, stop_label_offset;
        svg::Color underlayer_color;
        std::vector<svg::Color> color_palette;
    };

    class MapRenderer
    {
    public:
        MapRenderer() = default;

        MapRenderer(RenderSettings render_settings);

        void SetRenderSettings(RenderSettings render_settings);

        svg::Document Render(const std::map<std::string_view, const Bus*>& buses) const;

    private:
        RenderSettings render_settings_;
    };
}