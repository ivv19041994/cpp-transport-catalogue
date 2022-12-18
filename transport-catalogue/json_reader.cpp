#include "json_reader.h"

#include "request_handler.h"
#include "map_renderer.h"

namespace transport {
namespace json {
	using namespace std;
	using ::json::Load;

	renderer::Color ParsingColor(const ::json::Node& color_node) {
		if (color_node.IsString()) {
			return renderer::Color{ color_node.AsString() };
		}
		if (color_node.IsArray()) {
			auto& arr = color_node.AsArray();
			if (arr.size() == 3) {
				return renderer::Rgb{
					static_cast<unsigned char>(arr.at(0).AsInt()),
					static_cast<unsigned char>(arr.at(1).AsInt()),
					static_cast<unsigned char>(arr.at(2).AsInt())
				};
			}

			if (arr.size() == 4) {
				return renderer::Rgba{
					static_cast<unsigned char>(arr.at(0).AsInt()),
					static_cast<unsigned char>(arr.at(1).AsInt()),
					static_cast<unsigned char>(arr.at(2).AsInt()),
					arr.at(3).AsDouble()
				};
			}
		}

		throw std::runtime_error("JSON color node invalid format: "s + color_node.Print());
	}

	transport::renderer::RenderSettings ParseRenderSettings(const ::json::Node& render_node) {
		const Dict& render_dict = render_node.AsMap();
		renderer::RenderSettings render_settings;
		render_settings.width = render_dict.at("width").AsDouble();
		render_settings.height = render_dict.at("height").AsDouble();
		render_settings.padding = render_dict.at("padding").AsDouble();

		render_settings.line_width = render_dict.at("line_width").AsDouble();
		render_settings.stop_radius = render_dict.at("stop_radius").AsDouble();

		render_settings.bus_label_font_size = render_dict.at("bus_label_font_size").AsInt();
		{
			auto& bus_label_offset = render_dict.at("bus_label_offset").AsArray();
			render_settings.bus_label_offset = renderer::Point{ bus_label_offset[0].AsDouble(),  bus_label_offset[1].AsDouble() };
		}

		render_settings.stop_label_font_size = render_dict.at("stop_label_font_size").AsInt();
		{
			auto& stop_label_offset = render_dict.at("stop_label_offset").AsArray();
			render_settings.stop_label_offset = renderer::Point{ stop_label_offset[0].AsDouble(),  stop_label_offset[1].AsDouble() };
		}

		render_settings.underlayer_color = ParsingColor(render_dict.at("underlayer_color"));
		render_settings.underlayer_width = render_dict.at("underlayer_width").AsDouble();

		auto& color_palette = render_dict.at("color_palette").AsArray();
		for (auto& color : color_palette) {
			render_settings.color_palette.push_back(ParsingColor(color));
		}
		return render_settings;
	}

	void InputStatReader::operator()(std::istream& is, std::ostream& os, transport::TransportCatalogue& transport_catalogue) {
	
	Document document = Load(is);
	const Node& root = document.GetRoot();

	if (root.AsMap().count("render_settings")) {
		render_settings_ = ParseRenderSettings(root.AsMap().at("render_settings"));
	}

	InputReader(root.AsMap().at("base_requests"), transport_catalogue);
	if (root.AsMap().count("stat_requests")) {
		StatReader(root.AsMap().at("stat_requests"), transport_catalogue).Print(os);
	}
}

void InputStatReader::InputReader(const Node& input_node, transport::TransportCatalogue& transport_catalogue) {
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

Node InputStatReader::BusRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {

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

::json::Node InputStatReader::StopRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {
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

::json::Node InputStatReader::MapRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {

	if (!render_settings_) {
		throw std::runtime_error("Map request without render_settings");
	}
	renderer::MapRenderer map_renderer{ *render_settings_ , transport_catalogue.GetBuses().begin(), transport_catalogue.GetBuses().end() };

	Dict result;
	result["request_id"] = request.at("id");
	result["map"] = map_renderer.Render();
	return Node(move(result));
}

::json::Node InputStatReader::StatRequest(const Dict& request, const TransportCatalogue& transport_catalogue) {
	auto& type = request.at("type");
	if (type == "Bus"s) {
		return BusRequest(request, transport_catalogue);
	}

	if (type == "Stop"s) {
		return StopRequest(request, transport_catalogue);
	}

	if (type == "Map"s) {
		return MapRequest(request, transport_catalogue);
	}
	return ::json::Node{ Dict {} };
}

::json::Node InputStatReader::StatReader(const ::json::Node& stat_node, const TransportCatalogue& transport_catalogue) {
	Array result{};
	for (auto& request : stat_node.AsArray()) {
		result.push_back(StatRequest(request.AsMap(), transport_catalogue));
	}
	return ::json::Node{ move(result) };
}



} //namespace json
} //namespace transport