#pragma once

#include "json.h"
#include "transport_catalogue.h"

namespace transport {
	namespace json {

		void InputStatReader(std::istream& is, std::ostream& os, transport::TransportCatalogue& transport_catalogue);

		std::istream& InputReader(std::istream& is, transport::TransportCatalogue& transport_catalogue);
		void InputReader(const ::json::Node& input_node, transport::TransportCatalogue& transport_catalogue);

		void StatReader(std::istream& is, std::ostream& os, TransportCatalogue& transport_catalogue);
		::json::Node StatReader(const ::json::Node& stat_node, TransportCatalogue& transport_catalogue);
	}
}