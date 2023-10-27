#include "map_renderer.h"
#include "geo.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>

using namespace transport;

namespace
{
    using namespace std::literals;
    const std::string DEFAULT_CIRCLE_COLOR{"white"s};
    const std::string DEFAULT_TEXT_COLOR{"black"s};
    const std::string DEFAULT_FONT_FAMILY{"Verdana"s};
    const std::string DEFAULT_FONT_WEIGHT{"bold"s};

    inline const double EPSILON = 1e-6;

    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    class SphereProjector {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template<typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding)
                : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.second.lng < rhs.second.lng; });
            min_lon_ = left_it->second.lng;
            const double max_lon = right_it->second.lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.second.lat < rhs.second.lat; });
            const double min_lat = bottom_it->second.lat;
            max_lat_ = top_it->second.lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(transport::Coordinates coords) const {
            return {
                    (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    svg::Polyline GetBusPolyline(const Bus& bus, const RenderSettings& render_settings, size_t color_count, const SphereProjector& proj)
    {
        svg::Polyline polyline{};
        polyline.SetStrokeColor(render_settings.color_palette[color_count])
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(render_settings.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (const auto stop_ptr: bus.bus_stops)
        {
            polyline.AddPoint(proj(stop_ptr->coordinates));
        }

        if (!bus.is_roundtrip)
        {
            for (auto it = bus.bus_stops.rbegin() + 1; it != bus.bus_stops.rend(); ++it)
            {
                polyline.AddPoint(proj((*it)->coordinates));
            }
        }

        return polyline;
    }

    svg::Text GetBusNameText(const std::string& name, svg::Point pos, const RenderSettings& render_settings)
    {
        return svg::Text()
                    .SetFillColor(render_settings.underlayer_color)
                    .SetStrokeColor(render_settings.underlayer_color)
                    .SetStrokeWidth(render_settings.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                    .SetPosition(pos)
                    .SetOffset({render_settings.bus_label_offset.first, render_settings.bus_label_offset.second})
                    .SetFontSize(render_settings.bus_label_font_size)
                    .SetFontFamily(DEFAULT_FONT_FAMILY)
                    .SetFontWeight(DEFAULT_FONT_WEIGHT)
                    .SetData(name);
    }

    svg::Text GetBusNameTextBackground(const std::string& name, svg::Point pos, const RenderSettings& render_settings, size_t color_count)
    {
        return svg::Text()
                    .SetFillColor(render_settings.color_palette[color_count])
                    .SetPosition(pos)
                    .SetOffset({render_settings.bus_label_offset.first, render_settings.bus_label_offset.second})
                    .SetFontSize(render_settings.bus_label_font_size)
                    .SetFontFamily(DEFAULT_FONT_FAMILY)
                    .SetFontWeight(DEFAULT_FONT_WEIGHT)
                    .SetData(name);
    }

    void AddBusNameText(svg::Document& document, const Bus& bus, const RenderSettings& render_settings, size_t color_count, const SphereProjector& proj)
    {
        const auto first_stop = *bus.bus_stops.begin();
        const auto last_stop = *bus.bus_stops.rbegin();
        svg::Point first_pos = proj(first_stop->coordinates);

        document.Add(GetBusNameText(bus.name, first_pos, render_settings));
        document.Add(GetBusNameTextBackground(bus.name, first_pos, render_settings, color_count));

        if (first_stop != last_stop)
        {
            svg::Point last_pos = proj(last_stop->coordinates);
            document.Add(GetBusNameText(bus.name, last_pos, render_settings));
            document.Add(GetBusNameTextBackground(bus.name, last_pos, render_settings, color_count));
        }
    }

    void AddStopNameText(svg::Document& document, std::string_view name, svg::Point pos, const RenderSettings& render_settings)
    {
        document.Add(svg::Text()
                             .SetFillColor(render_settings.underlayer_color)
                             .SetStrokeColor(render_settings.underlayer_color)
                             .SetStrokeWidth(render_settings.underlayer_width)
                             .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                             .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                             .SetPosition(pos)
                             .SetOffset({render_settings.stop_label_offset.first, render_settings.stop_label_offset.second})
                             .SetFontSize(render_settings.stop_label_font_size)
                             .SetFontFamily(DEFAULT_FONT_FAMILY)
                             .SetData(std::string{name}));

        document.Add(svg::Text()
                             .SetFillColor(DEFAULT_TEXT_COLOR)
                             .SetPosition(pos)
                             .SetOffset({render_settings.stop_label_offset.first, render_settings.stop_label_offset.second})
                             .SetFontSize(render_settings.stop_label_font_size)
                             .SetFontFamily(DEFAULT_FONT_FAMILY)
                             .SetData(std::string{name}));
    }
}

MapRenderer::MapRenderer(RenderSettings render_settings)
    : render_settings_(std::move(render_settings))
{}

void MapRenderer::SetRenderSettings(RenderSettings render_settings)
{
    render_settings_ = std::move(render_settings);
}

svg::Document MapRenderer::Render(const std::map<std::string_view, const Bus*>& buses) const
{
    svg::Document document;
    std::map<std::string_view, Coordinates> name_to_coordinates;

    for (const auto& [bus_name, bus_ptr] : buses)
    {
        for (const auto stop_ptr : bus_ptr->bus_stops)
        {
            name_to_coordinates.emplace(stop_ptr->name, stop_ptr->coordinates);
        }
    }

    const SphereProjector proj{
                        name_to_coordinates.begin(), name_to_coordinates.end(),
                        render_settings_.width,
                        render_settings_.height,
                        render_settings_.padding
    };

    size_t palette_size = render_settings_.color_palette.size();
    size_t color_count = palette_size;

    for (const auto& [bus_name, bus_ptr] : buses)
    {
        if (bus_ptr->bus_stops.empty())
        {
            continue;
        }

        ++color_count;
        if (color_count >= palette_size)
        {
            color_count = 0;
        }

        document.Add(GetBusPolyline(*bus_ptr, render_settings_, color_count, proj));
    }

    color_count = palette_size;

    for (const auto& [bus_name, bus_ptr] : buses)
    {
        if (bus_ptr->bus_stops.empty())
        {
            continue;
        }

        ++color_count;
        if (color_count >= palette_size)
        {
            color_count = 0;
        }

        AddBusNameText(document, *bus_ptr, render_settings_, color_count, proj);
    }

    for (const auto& [name, coordinates] : name_to_coordinates)
    {
        document.Add(svg::Circle()
                                    .SetCenter(proj(coordinates))
                                    .SetRadius(render_settings_.stop_radius)
                                    .SetFillColor(DEFAULT_CIRCLE_COLOR));
    }

    for (const auto& [name, coordinates] : name_to_coordinates)
    {
        AddStopNameText(document, name, proj(coordinates), render_settings_);
    }

    return document;
}