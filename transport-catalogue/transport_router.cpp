#include "transport_router.h"

#include <numeric>

#include <iostream>
#include <memory>

namespace transport {
namespace router {
Router::Router(const TransportCatalogue& transport_catalogue, RouterSettings settings)
	: graph_{ std::make_unique<Graph>(GraphBuilder(transport_catalogue, settings).Build())}
	, router_{ graph_->directed_weighted_graph } {
}

Router::Router( Graph graph)
	: graph_{ std::make_unique<Graph>(std::move(graph))}
	, router_{ graph_->directed_weighted_graph } {

}

const Graph& Router::GetGraph() const {
	return *graph_;
}

std::optional<Router::RouteInfo> Router::BuildRoute(size_t from_index, size_t to_index) const {
	auto route = router_.BuildRoute(from_index, to_index);
	if (!route) {
		return std::nullopt;
	}

	std::vector<Event> events;

	for (graph::EdgeId edge_id : route.value().edges) {
		auto& edge = graph_->edges.at(edge_id);
		events.push_back(edge.wait);
		events.push_back(edge.span);
	}

	return Router::RouteInfo{ route.value().weight, std::move(events) };
}

GraphBuilder::GraphBuilder(const TransportCatalogue& transport_catalogue, RouterSettings settings)
	: transport_catalogue_{ transport_catalogue }
	, settings_{ settings }
	, bus_velocity_meters_per_min_{ settings_.bus_velocity * 1000 / 60 } {}

Graph GraphBuilder::Build() const {
	Graph graph{
		graph::DirectedWeightedGraph<Time> {GetVertexCount()},
		{}
	};

	auto& buses = transport_catalogue_.GetBuses();

	for (const auto& bus : buses) {
		AddForwardBusTrips(bus, graph);
		if (!bus.circular_)
			AddBackwardBusTrips(bus, graph);
	}

	return graph;
}

size_t GraphBuilder::GetVertexCount() const {
	size_t ret = transport_catalogue_.GetStops().size();
	return ret;
}

template<typename StopForwardIt>
void GraphBuilder::AddBusTrips(const Bus& bus, Graph& graph, StopForwardIt stop_begin, StopForwardIt stop_end) const {

	std::vector<std::tuple<graph::VertexId, Time, size_t>> to_time_spans;

	for (auto from_it = stop_begin, to_it = from_it++; from_it != stop_end; ++to_it, ++from_it) {
		Time from_to_time = GetTime(*from_it, *to_it);
		graph::VertexId to_id = transport_catalogue_.GetStopIndex((*to_it)->name_);// stop_to_vertex_.at(*to_it);
		graph::VertexId from_id = transport_catalogue_.GetStopIndex((*from_it)->name_);//stop_to_vertex_.at(*from_it);
		for (auto& [id, time, spans] : to_time_spans) {
			time += from_to_time;
			++spans;
		}
		to_time_spans.push_back({ to_id , from_to_time, 1 });
		for (auto& [id, time, spans] : to_time_spans) {
			if (id != from_id) {
				AddEdge(graph, from_id, id, {
						Wait{ transport_catalogue_.GetStopIndex((*from_it)->name_), settings_.bus_wait_time},
						Span{ transport_catalogue_.GetBusIndex(bus.name_), time, spans}
					}
				);
			}
		}
	}
}


void GraphBuilder::AddForwardBusTrips(const Bus& bus, Graph& graph) const {

	auto& stops = bus.stops_;
	AddBusTrips(bus, graph, stops.rbegin(), stops.rend());
}

void GraphBuilder::AddBackwardBusTrips(const Bus& bus, Graph& graph) const {

	auto& stops = bus.stops_;
	AddBusTrips(bus, graph, stops.begin(), stops.end());
}

Time GraphBuilder::GetTime(const Stop* from, const Stop* to) const {
	auto length_meters = transport_catalogue_.GetLengthFromTo(from, to);
	return static_cast<double>(length_meters) / bus_velocity_meters_per_min_;
}

void GraphBuilder::AddEdge(Graph& graph, graph::VertexId from, graph::VertexId to, EdgeInfo edge) const {

	graph.edges.push_back(std::move(edge));
	try {
		auto& edge = graph.edges.back();
		graph.directed_weighted_graph.AddEdge({
			from,
			to,
			edge.wait.time + edge.span.time
			});
	}
	catch (const std::exception& e) {
		graph.edges.pop_back();
		throw e;
	}
	catch (...) {
		throw - 1;
	}
}

} // namespace router
} // namespace transport