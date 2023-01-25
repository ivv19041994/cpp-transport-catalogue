#include "map_renderer.h"
#include <algorithm>
#include <sstream>
namespace transport {

namespace renderer {


svg::Color MapRender::ColorToSvg(const Color& color) {
    using namespace std::string_literals;

    if (std::holds_alternative<std::string>(color)) {
        return std::get<std::string>(color);
    }

    if (std::holds_alternative<Rgb>(color)) {
        const Rgb& rgb = std::get<Rgb>(color);
        return "rgb("s +
            std::to_string(rgb.r) + ","s +
            std::to_string(rgb.g) + ","s +
            std::to_string(rgb.b) + ")"s;
    }
    const Rgba& rgba = std::get<Rgba>(color);
    std::stringstream s;
    s << "rgba("s <<
        static_cast<int>(rgba.rgb.r) << "," <<
        static_cast<int>(rgba.rgb.g) << "," <<
        static_cast<int>(rgba.rgb.b) << "," <<
        rgba.opacity << ")"s;
    return s.str();
}

void MapRender::Render(std::ostream& os) const {
    document_.Render(os);
}

std::string MapRender::Render() const {
    std::stringstream result;
    document_.Render(result);
    return result.str();
}

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRender::AddLines() {
    size_t color_palette_counter = 0;
    for (const Bus* bus : buses_) {
        if (bus->stops_.size()) {
            svg::Polyline polyline;
            for (const Stop* stop : bus->stops_) {
                polyline.AddPoint((*sphere_projector_)(stop->coordinates_));
            }
            if (!bus->circular_) {
                auto it = bus->stops_.crbegin();
                for (++it; it != bus->stops_.crend(); ++it) {
                    polyline.AddPoint((*sphere_projector_)((*it)->coordinates_));
                }
            }
            polyline.
                SetFillColor(svg::NoneColor).
                SetStrokeColor(color_palette_[color_palette_counter]).
                SetStrokeWidth(line_width_).
                SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);


            color_palette_counter = (color_palette_counter + 1) % color_palette_.size();
            document_.Add(std::move(polyline));
        }
    }
}

void MapRender::FillColorPalette(const std::vector<Color>& color_palette) {
    for (auto& color : color_palette) {
        color_palette_.push_back(ColorToSvg(color));
    }
}

void MapRender::SetCommonBusTextSettings(svg::Text& text, const std::string_view bus_name, const svg::Point& position) {
    text.SetPosition(position)
        .SetOffset(bus_label_offset_)
        .SetFontSize(bus_label_font_size_)
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetData(std::string(bus_name));
}

void MapRender::AddBusName(const std::string_view name, const Stop* stop, const svg::Color& color) {
    svg::Text substrate;
    svg::Text text;

    svg::Point position = (*sphere_projector_)(stop->coordinates_);

    SetCommonBusTextSettings(substrate, name, position);
    SetCommonBusTextSettings(text, name, position);

    substrate.SetFillColor(underlayer_color_)
        .SetStrokeColor(underlayer_color_)
        .SetStrokeWidth(underlayer_width_)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    text.SetFillColor(color);
    document_.Add(std::move(substrate));
    document_.Add(std::move(text));
}

void MapRender::AddBusNames() {
    size_t color_palette_counter = 0;
    for (const Bus* bus : buses_) {
        if (bus->stops_.size()) {

            AddBusName(bus->name_, bus->stops_.front(), color_palette_.at(color_palette_counter));
            if (bus->stops_.front() != bus->stops_.back()) {
                AddBusName(bus->name_, bus->stops_.back(), color_palette_.at(color_palette_counter));
            }

            color_palette_counter = (color_palette_counter + 1) % color_palette_.size();
        }
    }
}

void MapRender::AddStopRounds() {
    for (const Stop* stop : stops_with_buses_) {

        svg::Circle circle;

        circle.SetCenter((*sphere_projector_)(stop->coordinates_))
            .SetRadius(stop_radius_)
            .SetFillColor("white");

        document_.Add(std::move(circle));
    }
    
}

void MapRender::SetCommonStopTextSettings(svg::Text& text, const std::string_view stop_name, const svg::Point& position) {
    text.SetPosition(position)
        .SetOffset(stop_label_offset_)
        .SetFontSize(stop_label_font_size_)
        .SetFontFamily("Verdana")
        .SetData(std::string(stop_name));
}

void MapRender::AddStopNames() {
    for (const Stop* stop : stops_with_buses_) {
        svg::Text substrate;
        svg::Text text;

        svg::Point position = (*sphere_projector_)(stop->coordinates_);

        SetCommonStopTextSettings(substrate, stop->name_, position);
        SetCommonStopTextSettings(text, stop->name_, position);

        substrate.SetFillColor(underlayer_color_)
            .SetStrokeColor(underlayer_color_)
            .SetStrokeWidth(underlayer_width_)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        text.SetFillColor("black");

        document_.Add(std::move(substrate));
        document_.Add(std::move(text));
    }
}

}//namespace renderer

}//namespace transport