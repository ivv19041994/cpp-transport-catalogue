#include "transport_router.h"

#include <numeric>

#include <iostream>

namespace transport {
	namespace router {
		Router::Router(const TransportCatalogue& transport_catalogue, RouterSettings settings)
			: transport_catalogue_{ transport_catalogue }
			, settings_{ settings }
			, bus_velocity_meters_per_min_{ settings_.bus_velocity * 1000 / 60 }
			//, stop_to_vertex_{ StopToVertex() }
			, graph_{ BuildGraph() }
			, router_{ graph_.directed_weighted_graph } {
		}

		Router::Router(const TransportCatalogue& transport_catalogue, RouterSettings settings, Graph graph) : transport_catalogue_{ transport_catalogue }
			, settings_{ settings }
			, bus_velocity_meters_per_min_{ settings_.bus_velocity * 1000 / 60 }
			//, stop_to_vertex_{ StopToVertex() }
			, graph_{ std::move(graph)}
			, router_{ graph_.directed_weighted_graph } {

		}

		const RouterSettings& Router::GetSettings() const {
			return settings_;
		}

		const Router::Graph& Router::GetGraph() const {
			return graph_;
		}

		const TransportCatalogue& Router::GetTransportCatalogue() const {
			return transport_catalogue_;
		}

		size_t Router::GetVertexCount() const {
			size_t ret = transport_catalogue_.GetStops().size();

			/*auto& buses = transport_catalogue_.GetBuses();

			for (auto& bus : buses) {
				ret += bus.stops_.size() + (bus.circular_ ? -1 : 0);
			}*/

			return ret;
		}

		std::unordered_map<const Stop*, graph::VertexId> Router::StopToVertex() const {
			std::unordered_map<const Stop*, graph::VertexId> ret;
			auto& stops = transport_catalogue_.GetStops();
			graph::VertexId id = 0;
			for (auto& stop : stops) {
				ret[&stop] = id++;
			}

			return ret;
		}
		void Router::AddEdge(Graph& graph, graph::VertexId from, graph::VertexId to, EdgeInfo edge) const {

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

		Time Router::GetTime(const Stop* from, const Stop* to) const {
			auto length_meters = transport_catalogue_.GetLengthFromTo(from, to);
			return static_cast<double>(length_meters) / bus_velocity_meters_per_min_;
		}

		template<typename StopForwardIt>
		void Router::AddBusTrips(const Bus& bus, Graph& graph, StopForwardIt stop_begin, StopForwardIt stop_end) const {

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
								Wait{*from_it, settings_.bus_wait_time},
								Span{&bus, time, spans }
							}
						);
					}
				}
			}
		}


		void Router::AddForwardBusTrips(const Bus& bus, Graph& graph) const {

			auto& stops = bus.stops_;
			AddBusTrips(bus, graph, stops.rbegin(), stops.rend());
		}

		void Router::AddBackwardBusTrips(const Bus& bus, Graph& graph) const {

			auto& stops = bus.stops_;
			AddBusTrips(bus, graph, stops.begin(), stops.end());
		}



		Router::Graph Router::BuildGraph() const {

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

		std::optional<Router::RouteInfo> Router::BuildRoute(const Stop* from, const Stop* to) const {
			auto route = router_.BuildRoute(transport_catalogue_.GetStopIndex(from->name_), transport_catalogue_.GetStopIndex(to->name_));
			if (!route) {
				return std::nullopt;
			}

			std::vector<Event> events;

			for (graph::EdgeId edge_id : route.value().edges) {
				auto& edge = graph_.edges.at(edge_id);
				events.push_back(edge.wait);
				events.push_back(edge.span);
			}

			return Router::RouteInfo{ route.value().weight, std::move(events) };
		}
	} // namespace router
} // namespace transport