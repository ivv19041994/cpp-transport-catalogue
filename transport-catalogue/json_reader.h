#pragma once

#include <optional>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"


namespace transport {
	namespace json {

		using ::json::Node;
		using ::json::Document;
		using ::json::Array;
		using ::json::Dict;

		class InputStatReader {
		public:
			void operator()(std::istream& is, std::ostream& os, transport::TransportCatalogue& transport_catalogue);
		private:
			std::optional<::transport::renderer::RenderSettings> render_settings_;

			void InputReader(const Node& input_node, transport::TransportCatalogue& transport_catalogue);
			Node StatReader(const Node& stat_node, const TransportCatalogue& transport_catalogue);

			Node StatRequest(const Dict& request, const TransportCatalogue& transport_catalogue);
			Node BusRequest(const Dict& request, const TransportCatalogue& transport_catalogue);
			Node StopRequest(const Dict& request, const TransportCatalogue& transport_catalogue);
			Node MapRequest(const Dict& request, const TransportCatalogue& transport_catalogue);
		};
	}
}