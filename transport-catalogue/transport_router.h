#pragma once

#include "transport_catalogue.h"
#include <variant>
#include "router.h"


namespace transport {

namespace router {

	using Time = double;
	using Speed = double;

	struct RouterSettings {
		Time bus_wait_time = 6;
		Speed bus_velocity = 40;
	};

	struct Wait {
		size_t stop;
		Time time;
	};

	struct Span {
		size_t bus;
		Time time;
		size_t count;
	};

	struct EdgeInfo {
		Wait wait;
		Span span;
	};

	struct Graph {
		graph::DirectedWeightedGraph<Time> directed_weighted_graph;
		std::vector<EdgeInfo> edges;
	};

	class GraphBuilder {
	public:
		GraphBuilder(const TransportCatalogue& transport_catalogue, RouterSettings settings);
		Graph Build() const;
	private:
		const TransportCatalogue& transport_catalogue_;
		RouterSettings settings_;
		Speed bus_velocity_meters_per_min_;

		size_t GetVertexCount() const;

		template<typename StopForwardIt>
		void AddBusTrips(const Bus& bus, Graph& graph, StopForwardIt stop_begin, StopForwardIt stop_end) const;
		void AddForwardBusTrips(const Bus& bus, Graph& graph) const;
		void AddBackwardBusTrips(const Bus& bus, Graph& graph) const;

		void AddEdge(Graph& graph, graph::VertexId from, graph::VertexId to, EdgeInfo edge) const;

		Time GetTime(const Stop* from, const Stop* to) const;
	};


	class Router {
	public:
		using Event = std::variant< Wait, Span>;


		Router(const TransportCatalogue& transport_catalogue, RouterSettings settings);
		Router(Graph graph);

		struct RouteInfo {
			Time total_time;
			std::vector<Event> events;
		};

		std::optional<RouteInfo> BuildRoute(size_t from_index, size_t to_index) const;

		const Graph& GetGraph() const;
	private:
		std::unique_ptr<Graph> graph_;//unique_ptr ����� router_ ����� ����������� �������� � ���������� ���������
		graph::Router<Time> router_;
	};
	
} // namespace router
} // namespace transport