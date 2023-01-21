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


		class Router {
		public:
			struct Wait {
				const Stop* stop;
				Time time;
			};

			struct Exit {
				const Bus* bus;
			};

			struct Span {
				const Bus* bus;
				Time time;
				size_t count = 1;
			};

			using Edge = std::variant< Wait, Exit, Span>;


			Router(const TransportCatalogue& transport_catalogue, RouterSettings settings);

			struct RouteInfo {
				Time total_time;
				std::vector<Edge> edges;
			};

			std::optional<RouteInfo> BuildRoute(const Stop* from, const Stop* to) const;

		private:

			struct Graph {
				graph::DirectedWeightedGraph<Time> directed_weighted_graph;
				std::vector<Edge> edges;
			};

			const TransportCatalogue& transport_catalogue_;
			RouterSettings settings_;
			std::unordered_map<const Stop*, graph::VertexId> stop_to_vertex_;
			Graph graph_;
			graph::Router<Time> router_;

			Graph BuildGraph() const;
			size_t GetVertexCount() const;
			graph::VertexId AddWaitEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const;
			void AddExitEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const;

			Time GetTime(const Stop* from, const Stop* to) const;

			template<typename StopForwardIt, typename VarexIdForwardIt>
			void AddSpanEdges(const Bus& bus, Graph& graph, StopForwardIt stop_begin, StopForwardIt stop_end, VarexIdForwardIt vartex_it) const;
			void AddSpanForwardEdges(const Bus& bus, Graph& graph, graph::VertexId start_vertex_id) const;
			void AddSpanBackwardEdges(const Bus& bus, Graph& graph, graph::VertexId end_vertex_id) const;

			template<class T>
			void AddEdge(Graph& graph, graph::VertexId from, graph::VertexId to, Time time, T edge) const;

			std::unordered_map<const Stop*, graph::VertexId> StopToVertex() const;
		};
	
	} // namespace router
} // namespace transport