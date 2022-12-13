#include "json_reader.h"

namespace transport {
namespace json {
	using namespace std;
	using ::json::Document;
	using ::json::Node;
	using ::json::Load;
	using ::json::Array;
	using ::json::Dict;
void InputStatReader(std::istream& is, std::ostream& os, transport::TransportCatalogue& transport_catalogue) {
	Document document = Load(is);
	const Node& root = document.GetRoot();
    //throw std::runtime_error(Print(root));

	InputReader(root.AsMap().at("base_requests"), transport_catalogue);
	StatReader(root.AsMap().at("stat_requests"), transport_catalogue).Print(os);
    
}

std::istream& InputReader(std::istream& is, transport::TransportCatalogue& transport_catalogue) {
	Document document = Load(is);
	InputReader(document.GetRoot(), transport_catalogue);
	return is;
}
void InputReader(const Node& input_node, transport::TransportCatalogue& transport_catalogue) {
	const Array& buses_stops = input_node.AsArray();
	vector<const Node*> buses;
	vector<const Node*> stops;
	buses.reserve(buses_stops.size());
	stops.reserve(buses_stops.size());
	//unordered_map<string, unordered_map<string, size_t>> length_from_to;
	for (const Node& node: buses_stops) {
		const Dict& dict = node.AsMap();

		if (dict.at("type") == "Stop"s) {
			stops.push_back(&node);
			transport_catalogue.AddStop(dict.at("name").AsString(), 
				geo::Coordinates{ dict.at("latitude").AsDouble(), dict.at("longitude").AsDouble() });
		} else if(dict.at("type") == "Bus"s) {
			buses.push_back(&node);
		}
	}

	for (auto& bus : buses) {
		const Dict& dict = bus->AsMap();
		std::vector<std::string> stop_names;
		const Array& stops = dict.at("stops").AsArray();
		stop_names.resize(stops.size());

		transform(stops.begin(), stops.end(), stop_names.begin(), [](auto& node) {
			return node.AsString();
			});

		transport_catalogue.AddBus(dict.at("name").AsString(), dict.at("is_roundtrip").AsBool(), stop_names);
	}

	std::unordered_map<std::string, std::unordered_map<std::string, size_t>> length_from_to;
	for (auto& stop : stops) {
		const Dict& dict = stop->AsMap();
		const Dict& other_stops = dict.at("road_distances").AsMap();
		const string& name = dict.at("name").AsString();
		for (auto& [other_name, node_len] : other_stops) {
			length_from_to[name][other_name] = node_len.AsInt();
		}
	}

	transport_catalogue.SetLengthBetweenStops(length_from_to);
}

void StatReader(std::istream& is, std::ostream& os, TransportCatalogue& transport_catalogue) {
	Document document = Load(is);
	StatReader(document.GetRoot(), transport_catalogue).Print(os);
}

::json::Node BusRequest(const Dict& request, TransportCatalogue& transport_catalogue) {
	//{
	//	"id": 12345678,
	//		"type" : "Bus",
	//		"name" : "14"
	//}
	Dict result;
	result["request_id"] = request.at("id");

	const Bus* bus = transport_catalogue.GetBus(request.at("name").AsString());
	if (!bus) {

		//{
		//	"request_id": 12345678,
		//		"error_message" : "not found"
		//}
		result["error_message"] = Node("not found"s);
		return Node(move(result));
	}

	//{
	//	"curvature": 2.18604,
	//		"request_id" : 12345678,
	//		"route_length" : 9300,
	//		"stop_count" : 4,
	//		"unique_stop_count" : 3
	//}
	auto route_length = transport_catalogue.GetLength(bus);
	result["curvature"] = Node(route_length / transport_catalogue.GetGeoLength(bus));
	result["route_length"] = Node(route_length);
	result["stop_count"] = Node(transport_catalogue.GetStopsCount(bus));
	result["unique_stop_count"] = Node(transport_catalogue.GetUniqueStopsCount(bus));
	return Node(move(result));
}

::json::Node StopRequest(const Dict& request, TransportCatalogue& transport_catalogue) {
	//{
	//	"id": 12345,
	//		"type" : "Stop",
	//		"name" : "Улица Докучаева"
	//}
	Dict result;
	result["request_id"] = request.at("id");

	const Stop* stop = transport_catalogue.GetStop(request.at("name").AsString());
	if (!stop) {
		//{
		//	"request_id": 12345,
		//		"error_message" : "not found"
		//}
		result["error_message"] = Node("not found"s);
		return Node(move(result));
	}
	//{
	//	"buses": [
	//		"14", "22к"
	//	] ,
	//		"request_id" : 12345
	//}
	Array buses_array;
	auto buses = transport_catalogue.GetBusesNamesFromStop(stop); //stop->buses_;
	if (buses.size()) {
		for (auto& name : buses) {
			buses_array.push_back(Node(name));
		}
	}
	result["buses"] = Node(move(buses_array));
	return Node(move(result));
}

::json::Node StatRequest(const Dict& request, TransportCatalogue& transport_catalogue) {
	auto& type = request.at("type");
	if (type == "Bus"s) {
		return BusRequest(request, transport_catalogue);
	}

	if (type == "Stop"s) {
		return StopRequest(request, transport_catalogue);
	}
	return ::json::Node{ Dict {} };
}

::json::Node StatReader(const ::json::Node& stat_node, TransportCatalogue& transport_catalogue) {
	Array result{};
	for (auto& request : stat_node.AsArray()) {
		result.push_back(StatRequest(request.AsMap(), transport_catalogue));
	}
	return ::json::Node{ move(result) };
}

} //namespace json
} //namespace transport