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

    template <typename Nameble>
    struct NameLess {
        bool operator()(const Nameble* lhs, const Nameble* rhs) const {
            return lhs->name_ < rhs->name_;
        }
    };

    class MapRender {
    public:
        template <typename ForvardIt>
        MapRender(const RenderSettings& settings, ForvardIt begin_buses, ForvardIt end_buses);



        svg::Color ColorToSvg(const Color& color);

        void Render(std::ostream& os);
        std::string Render();

    private:
        svg::Document document_;
        std::unique_ptr<SphereProjector> sphere_projector_;
        std::vector<svg::Color> color_palette_;
        std::set<const Bus*, NameLess<Bus>> buses_;
        std::set<const Stop*, NameLess<Stop>> stops_with_buses_;
        double line_width_;
        svg::Point bus_label_offset_;
        size_t bus_label_font_size_;
        svg::Color underlayer_color_;
        double underlayer_width_;
        double stop_radius_;
        svg::Point stop_label_offset_;
        size_t stop_label_font_size_;

        void FillColorPalette(const std::vector<Color>& color_palette);
        void AddLines();

        void SetCommonBusTextSettings(svg::Text& text, const std::string_view bus_name, const svg::Point& position);
        void AddBusName(const std::string_view name, const Stop* stop, const svg::Color& color);
        void AddBusNames();

        void AddStopRounds();
        void SetCommonStopTextSettings(svg::Text& text, const std::string_view stop_name, const svg::Point& position);
        void AddStopNames();
    };



    template <typename ForvardIt>
    MapRender::MapRender(const RenderSettings& settings, ForvardIt begin_buses, ForvardIt end_buses)
        : line_width_{settings.line_width}
        , bus_label_offset_{ settings.bus_label_offset.x,settings.bus_label_offset.y }
        , bus_label_font_size_{ settings.bus_label_font_size }
        , underlayer_color_{ ColorToSvg(settings.underlayer_color) }
        , underlayer_width_{ settings.underlayer_width }
        , stop_radius_{ settings.stop_radius }
        , stop_label_offset_{ settings.stop_label_offset.x,settings.stop_label_offset.y }
        , stop_label_font_size_{ settings.stop_label_font_size } {
        std::list<geo::Coordinates> coordinates;

        for (auto it = begin_buses; it != end_buses; ++it) {
            buses_.insert((&(*it)));
            for (const Stop* stop : it->stops_set_) {
                coordinates.push_back(stop->coordinates_);
                stops_with_buses_.insert(stop);
            }
        }
        
        sphere_projector_ = std::make_unique<SphereProjector>(coordinates.begin(), coordinates.end(),
            settings.width, settings.height, settings.padding);
        
        FillColorPalette(settings.color_palette);

        AddLines();
        AddBusNames();
        AddStopRounds();
        AddStopNames();
    }
}//namespace renderer 
}//namespace transport 