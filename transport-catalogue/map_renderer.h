#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <variant>
#include <array>
#include <set>
#include <iostream>

namespace transport {

namespace renderer {

    struct Point {
        double x = 0;
        double y = 0;
    };

    struct Rgb {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    struct Rgba {
        Rgb rgb;
        double opacity;
    };

    using Color = std::variant<std::string, Rgb, Rgba>;

    struct RenderSettings
    {
        double width;
        double height;
        double padding;

        double line_width;
        double stop_radius;

        size_t bus_label_font_size;
        Point bus_label_offset;

        size_t stop_label_font_size;
        Point stop_label_offset;

        Color underlayer_color;
        double underlayer_width;

        std::vector<Color> color_palette;
    };



    inline const double EPSILON = 1e-6;
    bool IsZero(double value);

    class SphereProjector {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
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
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

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
            }
            else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(geo::Coordinates coords) const;

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    class MapRenderer {
    public:
        template <typename ForvardIt>
        MapRenderer(const RenderSettings& settings, ForvardIt begin_buses, ForvardIt end_buses);

        svg::Color ColorToSvg(const Color& color);

        void Render(std::ostream& os);

    private:
        svg::Document document_;
    };

    struct NameLess {
        bool operator()(const Bus* lhs, const Bus* rhs) const {
            return lhs->name_ < rhs->name_;
        }
    };

    template <typename ForvardIt>
    MapRenderer::MapRenderer(const RenderSettings& settings, ForvardIt begin_buses, ForvardIt end_buses) {
        std::list<geo::Coordinates> coordinates;
        std::set<const Bus*, NameLess> buses;

        
        for (auto it = begin_buses; it != end_buses; ++it) {
            buses.insert((&(*it)));
            for (const Stop* stop : it->stops_set_) {
                coordinates.push_back(stop->coordinates_);
            }
        }
        
        SphereProjector sphere_projector = SphereProjector(coordinates.begin(), coordinates.end(),
            settings.width, settings.height, settings.padding);
        
        std::vector<svg::Color> color_palette;
        for (auto& color : settings.color_palette) {
            color_palette.push_back(ColorToSvg(color));
        }
        size_t color_palette_counter = 0;
        for (const Bus* bus : buses) {
            if (bus->stops_.size()) {
                svg::Polyline polyline;
                for (const Stop* stop : bus->stops_) {
                    polyline.AddPoint(sphere_projector(stop->coordinates_));
                }
                if (!bus->circular_) {
                    auto it = bus->stops_.crbegin();
                    for (++it; it != bus->stops_.crend(); ++it) {
                        polyline.AddPoint(sphere_projector((*it)->coordinates_));
                    }
                }
                polyline.
                    SetFillColor(svg::NoneColor).
                    SetStrokeColor(color_palette[color_palette_counter]).
                    SetStrokeWidth(settings.line_width).
                    SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                    SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);


                color_palette_counter = (color_palette_counter + 1) % color_palette.size();
                document_.Add(std::move(polyline));
            }
        }
    }
}//namespace renderer 
}//namespace transport 