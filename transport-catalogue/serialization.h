#pragma once
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"

#include "map_renderer.h"

namespace transport {
namespace serialize {
	
	
	
transport::TransportCatalogue DeserializeBase(const Base& input);
Base SerializeBase(const transport::TransportCatalogue& input);

void SaveTransportCatalogueTo(
	const transport::TransportCatalogue& transport_catalogue, 
	const renderer::RenderSettings& render_settings, 
	std::ostream& output);

renderer::RenderSettings DeserializeRenderSettings(const RenderSettings& input);
RenderSettings SerializeRenderSettings(const renderer::RenderSettings& input);

}
}