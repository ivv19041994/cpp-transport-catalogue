#include "serialization.h"
#include <cstdint>



#include <iostream>

namespace transport {
namespace serialize {
	
void DeserializeBase(transport::TransportCatalogue& transport_catalogue, const Base& base) {
	
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
		/*std::cout << "AddBus |" << bus.name() << "|" <<  std::endl;
		std::cout << "is_roundtrip = " << bus.is_roundtrip() << std::endl;
		std::cout << "stop_names : " << std::endl;
		for(auto&n : stop_names) {
			std::cout << n << std::endl;
		}*/
		transport_catalogue.AddBus(bus.name(), bus.is_roundtrip(), stop_names);
	}
	
	/*for(const transport::Bus& bus: transport_catalogue.GetBuses()) {
		std::cout << "AddBus |" << bus.name_ << "|" <<  std::endl;
		std::cout << "is_roundtrip = " << bus.circular_ << std::endl;
		std::cout << "stop_names : " << std::endl;
		for(auto&n : bus.stops_) {
			std::cout << n->name_ << std::endl;
		}
	}*/
	
}

transport::TransportCatalogue DeserializeBase(const Base& input) {
	transport::TransportCatalogue transport_catalogue;
	DeserializeBase(transport_catalogue, input);
	return transport_catalogue;
}


/*
transport::TransportCatalogue DeserializeTransportCatalogue(std::istream& input) {
	transport::TransportCatalogue transport_catalogue;
	transport::serialize::TransportCatalogue load;
	
	if (!load.ParseFromIstream(&input)) {
        return transport_catalogue;
    }
	
	if(!load.has_base()) {
		return transport_catalogue;
	}
	
	DeserializeBase(transport_catalogue, load.base());
	return transport_catalogue;
}*/

Stop Create(const transport::Stop stop) {
	Stop ret;
	ret.set_latitude(stop.coordinates_.lat);
	ret.set_longitude(stop.coordinates_.lng);
	ret.set_name(stop.name_);
	return ret;
}

template<typename C, typename T>
size_t GetContainerIndex(const C& container, const T* pointer) {
	size_t ret = 0;
	for (const auto& obj : container) {
		if (&obj == pointer) {
			return ret;
		}
		++ret;
	}
	assert(false);
	return ret;
}

Base SerializeBase(const transport::TransportCatalogue& transport_catalogue) {
	Base base;
	
	const std::deque<transport::Stop>& stops = transport_catalogue.GetStops();
	for(auto& stop: stops) {
		*base.add_stop() = Create(stop);
	}
	const auto& length_map = transport_catalogue.GetLengthMap();
	
	//size_t cnt = 0;
	std::unordered_map<const transport::Stop*, std::unordered_map<const transport::Stop*, size_t>> uniq_len;
	for(const auto&[from_to, len]: length_map) {
		const transport::Stop* from = from_to.first;
		const transport::Stop* to = from_to.second;
		auto it_to = uniq_len.find(to);
		if(it_to != uniq_len.end()) {
			auto it_from = it_to->second.find(from);
			if(it_from != it_to->second.end()) {
				if(it_from->second == len) {
					continue;
				}
			}
		}
		uniq_len[from][to] = len;
		//++cnt;
	}
	
	//std::cout << "uniq_len =" << cnt << " length_map = " << length_map.size() << std::endl;;
	
	for(const auto&[pfrom, umap]: uniq_len) {
		size_t from = transport_catalogue.GetStopIndex(pfrom->name_);
		
		for(const auto&[pto, len]: umap) {
			size_t to = transport_catalogue.GetStopIndex(pto->name_);
			uint32_t distance = (to & 0x7FF) | (static_cast<uint32_t>(len) << 11);
			base.mutable_stop(from)->add_road_distance(distance);
		}
	}
	
	for(const transport::Bus& bus: transport_catalogue.GetBuses()) {
		Bus b;
		b.set_name(bus.name_);
		b.set_is_roundtrip(bus.circular_);
	
		for(const transport::Stop* stop: bus.stops_) {
			auto stop_id = transport_catalogue.GetStopIndex(stop->name_);
			b.add_stop_id(stop_id);
		}
		*base.add_bus() = std::move(b);
	}
	
	return base;
}

void SaveTransportCatalogueTo(
	const transport::TransportCatalogue& transport_catalogue, 
	const renderer::RenderSettings& render_settings, 
	const router::Router& router,
	std::ostream& output) {
	transport::serialize::TransportCatalogue save;
	
	*save.mutable_base() = SerializeBase(transport_catalogue);
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

    ret.set_bus_label_font_size(input.bus_label_font_size);
    *ret.mutable_bus_label_offset() = SerializePoint(input.bus_label_offset);

    ret.set_stop_label_font_size(input.stop_label_font_size);
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
	ret.set_from_vertex(edge.from);
	ret.set_to_vertex(edge.to);
	ret.set_weight(edge.weight);
	return ret;
}

Graph SerializeGraph(const graph::DirectedWeightedGraph<double>& graph) {
	Graph ret;

	ret.set_vertex_count(graph.GetVertexCount());
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

WaitInfo SerializeWaitInfo(const router::Wait& wait, const transport::TransportCatalogue& tc) {
	WaitInfo ret;
	ret.set_stop_id(wait.stop);
	ret.set_time(wait.time);
	return ret;
}

SpanInfo SerializeSpanInfo(const router::Span& span, const transport::TransportCatalogue& tc) {
	SpanInfo ret;
	ret.set_bus_id(span.bus);
	ret.set_stop_count(span.count);
	ret.set_time_in_bus(span.time);
	return ret;
}


EdgeInfo SerializeEdgeInfo(const router::EdgeInfo& info, const transport::TransportCatalogue& tc) {
	EdgeInfo ret;
	*ret.mutable_wait_info() = SerializeWaitInfo(info.wait, tc);  //.set_wait_stop_id(GetStopIndex(stops, info.wait.stop));
	*ret.mutable_span_info() = SerializeSpanInfo(info.span, tc);
	return ret;
}

RouterGraph SerializeRouterGraph(const router::Graph& graph, const transport::TransportCatalogue& tc) {
	RouterGraph ret;
	*ret.mutable_graph() = SerializeGraph(graph.directed_weighted_graph);
	for (const auto& edge_info : graph.edges) {
		*ret.add_edge_info() = SerializeEdgeInfo(edge_info, tc);
	}
	return ret;
}

Router SerializeRouter(const router::Router& router) {
	Router ret;
	*ret.mutable_router_settings() = SerializeRouterSettings(router.GetSettings());
	*ret.mutable_graph() = SerializeRouterGraph(router.GetGraph(), router.GetTransportCatalogue());
	return ret;
}

router::RouterSettings DeserializeRouterSettings(const RouterSettings& router_settings) {
	router::RouterSettings ret;
	ret.bus_wait_time = router_settings.bus_wait_time_min();
	ret.bus_velocity = router_settings.bus_velocity_km_per_h();
	return ret;
}

router::Wait DeserializeWaitInfo(const WaitInfo& wait, const transport::TransportCatalogue& tc) {
	router::Wait ret;
	ret.stop = wait.stop_id();
	ret.time = wait.time();
	return ret;
}

router::Span DeserializeSpanInfo(const SpanInfo& span, const transport::TransportCatalogue& tc) {
	router::Span ret;

	ret.bus = span.bus_id();
	ret.count = span.stop_count();
	ret.time = span.time_in_bus();
	return ret;
}

router::EdgeInfo DeserializeEdgeInfo(const EdgeInfo& info, const transport::TransportCatalogue& tc) {
	router::EdgeInfo ret;
	ret.wait = DeserializeWaitInfo(info.wait_info(), tc);
	ret.span = DeserializeSpanInfo(info.span_info(), tc);
	return ret;
}

router::Graph DeserializeRouterGraph(const RouterGraph& graph, const transport::TransportCatalogue& tc) {
	router::Graph ret;
	ret.directed_weighted_graph = DeserializeGraph(graph.graph());

	for (int i = 0; i < graph.edge_info_size(); ++i) {
		ret.edges.push_back(DeserializeEdgeInfo(graph.edge_info(i), tc));
	}
	return ret;
}

router::Router DeserializeRouter(const Router& router, const transport::TransportCatalogue& tc) {
	return router::Router(
		tc, 
		DeserializeRouterSettings(router.router_settings()), 
		DeserializeRouterGraph(router.graph(), tc));
}


}
}