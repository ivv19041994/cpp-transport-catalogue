#include "serialization.h"

#include <cstdint>
#include <iostream>

namespace transport {
namespace serialize {
	
void DeserializeTransportCatalogue(transport::TransportCatalogue& transport_catalogue, const TransportCatalogue& base) {
	
	std::unordered_map<std::string, std::unordered_map<std::string, size_t>> length_from_to;
	for(int i = 0; i < base.stop_size(); ++i) {
		const Stop& stop = base.stop(i);
		transport_catalogue.AddStop(
			stop.name(),
			geo::Coordinates{ 
				stop.latitude(), 
				stop.longitude() 
			}
		);
		
		for(int length_ind = 0; length_ind < stop.road_distance_size(); ++length_ind) {
			uint32_t dis = stop.road_distance(length_ind);
			int other_id = dis & 0x7FF;
			const Stop& other = base.stop(other_id);
			length_from_to[stop.name()][other.name()] = dis >> 11;
		}
	}
	
	transport_catalogue.SetLengthBetweenStops(length_from_to);
	
	for(int bus_id = 0; bus_id < base.bus_size(); ++bus_id) {
		const Bus& bus = base.bus(bus_id);
		
		std::vector<std::string> stop_names;
		stop_names.reserve(bus.stop_id_size());
		
		for(int stop_index = 0; stop_index < bus.stop_id_size(); ++stop_index) {
			auto stop_id = bus.stop_id(stop_index);
			const Stop& stop = base.stop(stop_id);
			stop_names.push_back(stop.name());
		}

		transport_catalogue.AddBus(bus.name(), bus.is_roundtrip(), stop_names);
	}
}

transport::TransportCatalogue DeserializeTransportCatalogue(const TransportCatalogue& input) {
	transport::TransportCatalogue transport_catalogue;
	DeserializeTransportCatalogue(transport_catalogue, input);
	return transport_catalogue;
}

Stop SerializeStop(const transport::Stop stop) {
	Stop ret;
	ret.set_latitude(stop.coordinates_.lat);
	ret.set_longitude(stop.coordinates_.lng);
	ret.set_name(stop.name_);
	return ret;
}

std::unordered_map<const transport::Stop*, std::unordered_map<const transport::Stop*, size_t>> 
GetUniqueMapMap(const transport::TransportCatalogue& transport_catalogue) {
	const auto& length_map = transport_catalogue.GetLengthMap();

	std::unordered_map<const transport::Stop*, std::unordered_map<const transport::Stop*, size_t>> uniq_len;
	for (const auto& [from_to, len] : length_map) {
		const transport::Stop* from = from_to.first;
		const transport::Stop* to = from_to.second;
		auto it_to = uniq_len.find(to);
		if (it_to != uniq_len.end()) {
			auto it_from = it_to->second.find(from);
			if (it_from != it_to->second.end()) {
				if (it_from->second == len) {
					continue;
				}
			}
		}
		uniq_len[from][to] = len;
	}
	return uniq_len;
}

void AddRoadDistance(TransportCatalogue& output_tc, const transport::TransportCatalogue& transport_catalogue) {
	std::unordered_map<const transport::Stop*, std::unordered_map<const transport::Stop*, size_t>> uniq_len
		= GetUniqueMapMap(transport_catalogue);

	for (const auto& [pfrom, umap] : uniq_len) {
		size_t from = transport_catalogue.GetStopIndex(pfrom->name_);

		for (const auto& [pto, len] : umap) {
			size_t to = transport_catalogue.GetStopIndex(pto->name_);
			uint32_t distance = (static_cast<uint32_t>(to) & 0x7FF) | (static_cast<uint32_t>(len) << 11);
			output_tc.mutable_stop(static_cast<int>(from))->add_road_distance(distance);
		}
	}
}

void AddStops(TransportCatalogue& output_tc, const transport::TransportCatalogue& transport_catalogue) {
	const std::deque<transport::Stop>& stops = transport_catalogue.GetStops();
	for (auto& stop : stops) {
		*output_tc.add_stop() = SerializeStop(stop);
	}
}

void AddBuses(TransportCatalogue& output_tc, const transport::TransportCatalogue& transport_catalogue) {
	for (const transport::Bus& bus : transport_catalogue.GetBuses()) {
		Bus proto_bus;
		proto_bus.set_name(bus.name_);
		proto_bus.set_is_roundtrip(bus.circular_);

		for (const transport::Stop* stop : bus.stops_) {
			auto stop_id = transport_catalogue.GetStopIndex(stop->name_);
			proto_bus.add_stop_id(static_cast<uint32_t>(stop_id));
		}
		*output_tc.add_bus() = std::move(proto_bus);
	}
}

TransportCatalogue SerializeTransportCatalogue(const transport::TransportCatalogue& transport_catalogue) {
	TransportCatalogue ret_transport_catalogue;
	
	AddStops(ret_transport_catalogue, transport_catalogue);
	AddRoadDistance(ret_transport_catalogue, transport_catalogue);
	AddBuses(ret_transport_catalogue, transport_catalogue);
	
	return ret_transport_catalogue;
}

void SaveTransportCatalogueTo(
	const transport::TransportCatalogue& transport_catalogue, 
	const renderer::RenderSettings& render_settings, 
	const router::Router& router,
	std::ostream& output) {
	transport::serialize::Base save;
	
	*save.mutable_transport_catalogue() = SerializeTransportCatalogue(transport_catalogue);
	*save.mutable_render_settings() = SerializeRenderSettings(render_settings);
	*save.mutable_router() = SerializeRouter(router);

	save.SerializeToOstream(&output);
}

renderer::Point DeserializePoint(const Point& input) {
	return {input.x(), input.y()};
}

renderer::RenderSettings DeserializeRenderSettings(const RenderSettings& input) {
	renderer::RenderSettings ret;
	
    ret.width = input.width();
    ret.height = input.height();
    ret.padding = input.padding();

    ret.line_width = input.line_width();
    ret.stop_radius = input.stop_radius();

    ret.bus_label_font_size = input.bus_label_font_size();
    ret.bus_label_offset = DeserializePoint(input.bus_label_offset());

    ret.stop_label_font_size = input.stop_label_font_size();
    ret.stop_label_offset = DeserializePoint(input.stop_label_offset());

    ret.underlayer_color = input.underlayer_color();
    ret.underlayer_width = input.underlayer_width();
	
	ret.color_palette.reserve(input.color_palette_size());
	for(int color_index = 0; color_index < input.color_palette_size(); ++color_index) {
		ret.color_palette.push_back(input.color_palette(color_index));
	}
    return ret;
}

Point SerializePoint(const renderer::Point& input) {
	Point ret;
	ret.set_x(input.x);
	ret.set_y(input.y);
	return ret;
}

RenderSettings SerializeRenderSettings(const renderer::RenderSettings& input) {
	RenderSettings ret;
	
    ret.set_width(input.width);
    ret.set_height(input.height);
    ret.set_padding(input.padding);

    ret.set_line_width(input.line_width);
    ret.set_stop_radius(input.stop_radius);

    ret.set_bus_label_font_size(static_cast<uint32_t>(input.bus_label_font_size));
    *ret.mutable_bus_label_offset() = SerializePoint(input.bus_label_offset);

    ret.set_stop_label_font_size(static_cast<uint32_t>(input.stop_label_font_size));
    *ret.mutable_stop_label_offset() = SerializePoint(input.stop_label_offset);

    ret.set_underlayer_color(renderer::MapRender::ColorToSvg(input.underlayer_color));
    ret.set_underlayer_width(input.underlayer_width);
	
	for(const renderer::Color& color: input.color_palette) {
		*ret.add_color_palette() = renderer::MapRender::ColorToSvg(color);
	}
    return ret;
}

Edge SerializeEdge(const graph::Edge<double>& edge) {
	Edge ret;
	ret.set_from_vertex(static_cast<uint32_t>(edge.from));
	ret.set_to_vertex(static_cast<uint32_t>(edge.to));
	ret.set_weight(edge.weight);
	return ret;
}

Graph SerializeGraph(const graph::DirectedWeightedGraph<double>& graph) {
	Graph ret;

	ret.set_vertex_count(static_cast<uint32_t>(graph.GetVertexCount()));
	for (graph::EdgeId i = 0; i < graph.GetEdgeCount(); ++i) {
		*ret.add_edge() = SerializeEdge(graph.GetEdge(i));
	}
	return ret;
}

graph::Edge<double> DeserializeEdge(const Edge& edge) {
	graph::Edge<double> ret;
	ret.from = edge.from_vertex();
	ret.to = edge.to_vertex();
	ret.weight = edge.weight();
	return ret;
}

graph::DirectedWeightedGraph<double> DeserializeGraph(const Graph& graph) {
	graph::DirectedWeightedGraph<double> ret{graph.vertex_count()};
	for (int i = 0; i < graph.edge_size(); ++i) {
		ret.AddEdge(DeserializeEdge(graph.edge(i)));
	}
	return ret;
}

RouterSettings SerializeRouterSettings(const router::RouterSettings& router_settings) {
	RouterSettings ret;
	ret.set_bus_wait_time_min(router_settings.bus_wait_time);
	ret.set_bus_velocity_km_per_h(router_settings.bus_velocity);
	return ret;
}

WaitInfo SerializeWaitInfo(const router::Wait& wait) {
	WaitInfo ret;
	ret.set_stop_id(static_cast<uint32_t>(wait.stop));
	ret.set_time(wait.time);
	return ret;
}

SpanInfo SerializeSpanInfo(const router::Span& span) {
	SpanInfo ret;
	ret.set_bus_id(static_cast<uint32_t>(span.bus));
	ret.set_stop_count(static_cast<uint32_t>(span.count));
	ret.set_time_in_bus(span.time);
	return ret;
}


EdgeInfo SerializeEdgeInfo(const router::EdgeInfo& info) {
	EdgeInfo ret;
	*ret.mutable_wait_info() = SerializeWaitInfo(info.wait);  //.set_wait_stop_id(GetStopIndex(stops, info.wait.stop));
	*ret.mutable_span_info() = SerializeSpanInfo(info.span);
	return ret;
}

RouterGraph SerializeRouterGraph(const router::Graph& graph) {
	RouterGraph ret;
	*ret.mutable_graph() = SerializeGraph(graph.directed_weighted_graph);
	for (const auto& edge_info : graph.edges) {
		*ret.add_edge_info() = SerializeEdgeInfo(edge_info);
	}
	return ret;
}

Router SerializeRouter(const router::Router& router) {
	Router ret;
	*ret.mutable_graph() = SerializeRouterGraph(router.GetGraph());
	return ret;
}

router::RouterSettings DeserializeRouterSettings(const RouterSettings& router_settings) {
	router::RouterSettings ret;
	ret.bus_wait_time = router_settings.bus_wait_time_min();
	ret.bus_velocity = router_settings.bus_velocity_km_per_h();
	return ret;
}

router::Wait DeserializeWaitInfo(const WaitInfo& wait) {
	router::Wait ret;
	ret.stop = wait.stop_id();
	ret.time = wait.time();
	return ret;
}

router::Span DeserializeSpanInfo(const SpanInfo& span) {
	router::Span ret;

	ret.bus = span.bus_id();
	ret.count = span.stop_count();
	ret.time = span.time_in_bus();
	return ret;
}

router::EdgeInfo DeserializeEdgeInfo(const EdgeInfo& info) {
	router::EdgeInfo ret;
	ret.wait = DeserializeWaitInfo(info.wait_info());
	ret.span = DeserializeSpanInfo(info.span_info());
	return ret;
}

router::Graph DeserializeRouterGraph(const RouterGraph& graph) {
	router::Graph ret;
	ret.directed_weighted_graph = DeserializeGraph(graph.graph());

	for (int i = 0; i < graph.edge_info_size(); ++i) {
		ret.edges.push_back(DeserializeEdgeInfo(graph.edge_info(i)));
	}
	return ret;
}

router::Router DeserializeRouter(const Router& router) {
	return router::Router(
		DeserializeRouterGraph(router.graph()));
}

}
}