#pragma once
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"

#include "map_renderer.h"
#include "graph.h"
#include "transport_router.h"

namespace transport {
namespace serialize {
	
	
	
transport::TransportCatalogue DeserializeBase(const Base& input);
Base SerializeBase(const transport::TransportCatalogue& input);

void SaveTransportCatalogueTo(
	const transport::TransportCatalogue& transport_catalogue,
	const renderer::RenderSettings& render_settings,
	const router::Router& router,
	std::ostream& output);

renderer::RenderSettings DeserializeRenderSettings(const RenderSettings& input);
RenderSettings SerializeRenderSettings(const renderer::RenderSettings& input);

Graph SerializeGraph(const graph::DirectedWeightedGraph<double>& graph);
graph::DirectedWeightedGraph<double> DeserializeGraph(const Graph& graph);

Router SerializeRouter(const router::Router& router);
router::Router DeserializeRouter(const Router& router);

}
}