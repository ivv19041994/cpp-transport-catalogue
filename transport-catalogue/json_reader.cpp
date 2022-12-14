#include "json_reader.h"

#include "request_handler.h"

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

void StatReader(std::istream& is, std::ostream& os, const TransportCatalogue& transport_catalogue) {
	Document document = Load(is);
	StatReader(document.GetRoot(), transport_catalogue).Print(os);
}

::json::Node BusRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {

	RequestHandler request_handler{ transport_catalogue };
	//{
	//	"id": 12345678,
	//		"type" : "Bus",
	//		"name" : "14"
	//}
	Dict result;
	result["request_id"] = request.at("id");

	auto request_result = request_handler.GetBusStat(request.at("name").AsString());

	if (!request_result) {

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
	result["curvature"] = request_result->curvature;
	result["route_length"] = request_result->route_length;
	result["stop_count"] = request_result->stop_count;
	result["unique_stop_count"] = request_result->unique_stop_count;
	return Node(move(result));
}

::json::Node StopRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {
	RequestHandler request_handler{ transport_catalogue };
	//{
	//	"id": 12345,
	//		"type" : "Stop",
	//		"name" : "Улица Докучаева"
	//}
	Dict result;
	result["request_id"] = request.at("id");

	auto request_result = request_handler.GetSortedBusesByStop(request.at("name").AsString());

	if (!request_result) {
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

	for (auto& name : *request_result) {
		buses_array.push_back(Node(std::string(name)));
	}
	
	result["buses"] = Node(move(buses_array));
	return Node(move(result));
}

::json::Node StatRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {
	auto& type = request.at("type");
	if (type == "Bus"s) {
		return BusRequest(request, transport_catalogue);
	}

	if (type == "Stop"s) {
		return StopRequest(request, transport_catalogue);
	}
	return ::json::Node{ Dict {} };
}

::json::Node StatReader(const ::json::Node& stat_node, const TransportCatalogue& transport_catalogue) {
	Array result{};
	for (auto& request : stat_node.AsArray()) {
		result.push_back(StatRequest(request.AsMap(), transport_catalogue));
	}
	return ::json::Node{ move(result) };
}

} //namespace json
} //namespace transport