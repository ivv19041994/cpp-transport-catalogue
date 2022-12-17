#include "map_renderer.h"
#include <algorithm>
#include <sstream>
namespace transport {

namespace renderer {


svg::Color MapRenderer::ColorToSvg(const Color& color) {
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

void MapRenderer::Render(std::ostream& os) {
    document_.Render(os);
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

}//namespace renderer

}//namespace transport