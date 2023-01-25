#include "transport_router.h"

#include <numeric>

#include <iostream>

namespace transport {
	namespace router {
		Router::Router(const TransportCatalogue& transport_catalogue, RouterSettings settings)
			: transport_catalogue_{ transport_catalogue }
			, settings_{ settings }
			, stop_to_vertex_{ StopToVertex() }
			, graph_{ BuildGraph() }
			, router_{ graph_.directed_weighted_graph } {


		}

		size_t Router::GetVertexCount() const {
			size_t ret = transport_catalogue_.GetStops().size();

			auto& buses = transport_catalogue_.GetBuses();

			for (auto& bus : buses) {
				ret += bus.stops_.size() + (bus.circular_ ? -1 : 0);
			}

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

		template<class T>
		void Router::AddEdge(Graph& graph, graph::VertexId from, graph::VertexId to, Time time, T edge) const {

			graph.edges.push_back(std::move(edge));
			try {
				graph.directed_weighted_graph.AddEdge({
					from,
					to,
					time
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

		graph::VertexId Router::AddWaitEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const {

			auto end = bus.stops_.end();
			if (bus.circular_) {
				--end;
			}

			for (auto stop_it = bus.stops_.begin(); stop_it != end; ++stop_it, ++start_vertex_id) {
				AddEdge(
					graph,
					stop_to_vertex_.at(*stop_it),
					start_vertex_id,
					settings_.bus_wait_time,
					Wait{ *stop_it, settings_.bus_wait_time });
			}
			return start_vertex_id;
		}



		void Router::AddExitEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const {
			auto end = bus.stops_.end();
			auto begin = bus.stops_.begin();
			++start_vertex_id;//первую остановку и для кольца и для прямого маршрута пропускаем
			++begin;
			--end;
			for (; begin != end; ++begin, ++start_vertex_id) {
				AddEdge(
					graph,
					start_vertex_id,
					stop_to_vertex_.at(*begin),
					0,
					Exit{ &bus });
			}
		}

		Time Router::GetTime(const Stop* from, const Stop* to) const {
			auto length_meters = transport_catalogue_.GetLengthFromTo(from, to);
			return static_cast<double>(length_meters) / (settings_.bus_velocity * 1000 / 60);
		}

		template<typename StopForwardIt, typename VarexIdForwardIt>
		void Router::AddSpanEdges(const Bus& bus, Graph& graph, StopForwardIt stop_begin, StopForwardIt stop_end, VarexIdForwardIt vartex_it) const {

			--stop_end;//все кроме конечной, на ней надо принудительно выйти
			auto next_stop_it = stop_begin;
			auto stop_it = next_stop_it++;

			for (; next_stop_it != stop_end; ++stop_it, ++next_stop_it, ++vartex_it) {

				Time time = GetTime(*stop_it, *next_stop_it);
				AddEdge(
					graph,
					*vartex_it,
					*(vartex_it + 1),
					time,
					Span{ &bus, time });
			}

			//принудительно выходим на конечной:

			Time time = GetTime(*stop_it, *next_stop_it);
			AddEdge(
				graph,
				*vartex_it,
				stop_to_vertex_.at(*stop_end),
				time,
				Span{ &bus, time });
		}


		void Router::AddSpanForwardEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const {

			//auto r = std::views::iota(start_vertex_id, start_vertex_id + bus.stops_.size()); не работает в Яндекс
			std::vector<graph::VertexId> ids(bus.stops_.size());
			iota(ids.begin(), ids.end(), start_vertex_id);
			AddSpanEdges(bus, graph, bus.stops_.begin(), bus.stops_.end(), ids.begin());


			//auto end = bus.stops_.end();
			//--end;//все кроме конечной, на ней надо принудительно выйти
			//auto next_stop_it = bus.stops_.begin();
			//auto stop_it = next_stop_it++;

			////std::ranges::views::iota

			//for (; next_stop_it != end; ++stop_it, ++next_stop_it, ++start_vertex_id) {
			//	auto length_meters = transport_catalogue_.GetLengthFromTo(*stop_it, *next_stop_it);
			//	Time time = static_cast<double>(length_meters) / (settings_.bus_velocity * 1000 / 60);

			//	std::cout << "AddEdgeF " << start_vertex_id << " " << start_vertex_id + 1 << std::endl;
			//	AddEdge(
			//		graph,
			//		start_vertex_id,
			//		start_vertex_id + 1,
			//		time,
			//		Span{ &bus, time });
			//}

			////принудительно выходим на конечной:
			//auto length_meters = transport_catalogue_.GetLengthFromTo(*stop_it, *next_stop_it);
			//Time time = static_cast<double>(length_meters) / (settings_.bus_velocity * 1000 / 60);
			//std::cout << "AddEdgeF " << start_vertex_id << " " << stop_to_vertex_.at(*end) << std::endl;
			//AddEdge(
			//	graph,
			//	start_vertex_id,
			//	stop_to_vertex_.at(*end),
			//	time,
			//	Span{ &bus, time });
			//std::cout << "--------" << std::endl;
		}

		void Router::AddSpanBackwardEdges(const Bus& bus, Graph& graph, graph::VertexId end_vertex_id) const {

			//std::cout << "--------" << std::endl;
			//auto r = std::views::iota(end_vertex_id - bus.stops_.size(), end_vertex_id) | std::views::reverse; не работает в яндекс
			//AddSpanEdges(bus, graph, bus.stops_.rbegin(), bus.stops_.rend(), r.begin());

			std::vector<graph::VertexId> ids(bus.stops_.size());
			iota(ids.begin(), ids.end(), end_vertex_id - bus.stops_.size());

			AddSpanEdges(bus, graph, bus.stops_.rbegin(), bus.stops_.rend(), ids.rbegin());
			//std::cout << "--------" << std::endl;


			//auto end = bus.stops_.rend();
			//--end;//все кроме конечной, на ней надо принудительно выйти
			//--end_vertex_id;
			//auto next_stop_it = bus.stops_.rbegin();
			//auto stop_it = next_stop_it++;

			//for (; next_stop_it != end; ++stop_it, ++next_stop_it, --end_vertex_id) {
			//	auto length_meters = transport_catalogue_.GetLengthFromTo(*stop_it, *next_stop_it);
			//	Time time = static_cast<double>(length_meters) / (settings_.bus_velocity * 1000 / 60);

			//	std::cout << "AddEdgeB " << end_vertex_id << " " << end_vertex_id - 1 << std::endl;
			//	AddEdge(
			//		graph,
			//		end_vertex_id,
			//		end_vertex_id - 1,
			//		time,
			//		Span{ &bus, time });
			//}

			////принудительно выходим на конечной:
			//auto length_meters = transport_catalogue_.GetLengthFromTo(*stop_it, *next_stop_it);
			//Time time = static_cast<double>(length_meters) / (settings_.bus_velocity * 1000 / 60);
			//std::cout << "AddEdgeB " << end_vertex_id << " " << stop_to_vertex_.at(*end) << std::endl;
			//AddEdge(
			//	graph,
			//	end_vertex_id,
			//	stop_to_vertex_.at(*end),
			//	time,
			//	Span{ &bus, time });

			//std::cout << "--------" << std::endl;
		}



		Router::Graph Router::BuildGraph() const {

			Graph graph{
				graph::DirectedWeightedGraph<Time> {GetVertexCount()},
				{}
			};

			auto& buses = transport_catalogue_.GetBuses();
			graph::VertexId begin_vertex_id = stop_to_vertex_.size();

			for (auto& bus : buses) {
				graph::VertexId end_vertex_id = AddWaitEdges(bus, graph, begin_vertex_id);
				AddExitEdges(bus, graph, begin_vertex_id);
				AddSpanForwardEdges(bus, graph, begin_vertex_id);
				if (!bus.circular_)
					AddSpanBackwardEdges(bus, graph, end_vertex_id);
				begin_vertex_id = end_vertex_id;
			}

			return graph;
		}

		std::optional<Router::RouteInfo> Router::BuildRoute(const Stop* from, const Stop* to) const {
			auto route = router_.BuildRoute(stop_to_vertex_.at(from), stop_to_vertex_.at(to));
			if (!route) {
				return std::nullopt;
			}

			Span temp_span{ nullptr, 0, 0 };
			std::vector<Edge> edges;

			for (graph::EdgeId edge_id : route.value().edges) {
				const Edge& edge = graph_.edges.at(edge_id);

				if (const Span* pval = std::get_if<Span>(&edge)) {
					if (temp_span.bus != pval->bus) {
						if (temp_span.count) {
							edges.push_back(std::move(temp_span));
						}
						temp_span = *pval;
					}
					else {
						temp_span.count += pval->count;
						temp_span.time += pval->time;
					}
					continue;
				}
				if (temp_span.count) {
					edges.push_back(std::move(temp_span));
					temp_span = Span{ nullptr, 0, 0 };
				}

				if (const Wait* pval = std::get_if<Wait>(&edge)) {
					edges.push_back(*pval);
				}
			}

			if (temp_span.count) {
				edges.push_back(std::move(temp_span));
			}

			return Router::RouteInfo{ route.value().weight, std::move(edges) };
		}
	} // namespace router
} // namespace transport