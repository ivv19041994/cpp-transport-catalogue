#include "json_reader.h"

#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"


namespace transport {
namespace json {

using namespace std;
using ::json::Load;
using ::json::Builder;

renderer::Color ParseColor(const ::json::Node& color_node) {
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
	const Dict& render_dict = render_node.AsDict();
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

	render_settings.underlayer_color = ParseColor(render_dict.at("underlayer_color"));
	render_settings.underlayer_width = render_dict.at("underlayer_width").AsDouble();

	auto& color_palette = render_dict.at("color_palette").AsArray();
	for (auto& color : color_palette) {
		render_settings.color_palette.push_back(ParseColor(color));
	}
	return render_settings;
}

transport::router::RouterSettings ParseRouterSettings(const ::json::Node& settings_node) {
	const Dict& settings_dict = settings_node.AsDict();
	router::RouterSettings router_settings;

	router_settings.bus_velocity = settings_dict.at("bus_velocity").AsDouble();
	router_settings.bus_wait_time = settings_dict.at("bus_wait_time").AsDouble();

	return router_settings;
}

Base BaseReader::operator()(const Document& document) {
	const Node& root = document.GetRoot();
	const Dict& dict = root.AsDict();

	render_settings_ = ParseRenderSettings(dict.at("render_settings"));
	router_settings_ = ParseRouterSettings(dict.at("routing_settings"));
	InputReader(dict.at("base_requests"));
	router_.emplace(std::ref(transport_catalogue_), router_settings_);
	return {std::move(transport_catalogue_), render_settings_ , std::move(*router_)};
}

void BaseReader::InputReader(const Node& input_node) {
	const Array& buses_stops = input_node.AsArray();
	vector<const Node*> buses;
	vector<const Node*> stops;
	buses.reserve(buses_stops.size());
	stops.reserve(buses_stops.size());

	for (const Node& node : buses_stops) {
		const Dict& dict = node.AsDict();

		if (dict.at("type") == "Stop"s) {
			stops.push_back(&node);
			transport_catalogue_.AddStop(dict.at("name").AsString(),
				geo::Coordinates{ dict.at("latitude").AsDouble(), dict.at("longitude").AsDouble() });
		}
		else if (dict.at("type") == "Bus"s) {
			buses.push_back(&node);
		}
	}

	for (auto& bus : buses) {
		const Dict& dict = bus->AsDict();
		std::vector<std::string> stop_names;
		const Array& stops = dict.at("stops").AsArray();
		stop_names.resize(stops.size());

		transform(stops.begin(), stops.end(), stop_names.begin(), [](auto& node) {
			return node.AsString();
			});

		transport_catalogue_.AddBus(dict.at("name").AsString(), dict.at("is_roundtrip").AsBool(), stop_names);
	}

	std::unordered_map<std::string, std::unordered_map<std::string, size_t>> length_from_to;
	for (auto& stop : stops) {
		const Dict& dict = stop->AsDict();
		const Dict& other_stops = dict.at("road_distances").AsDict();
		const string& name = dict.at("name").AsString();
		for (auto& [other_name, node_len] : other_stops) {
			length_from_to[name][other_name] = node_len.AsInt();
		}
	}

	transport_catalogue_.SetLengthBetweenStops(length_from_to);
}

StatReader::StatReader(const Base& base) : base_{ base } {}

Document StatReader::operator()(const Document& document) {
	const Node& root = document.GetRoot();
	const Dict& dict = root.AsDict();

	return Document(StatRequests(dict.at("stat_requests")));
}

::json::Node StatReader::StatRequests(const Node& stat_node) {
	renderer::MapRender map_renderer{ 
		base_.render_settings, 
		base_.transport_catalogue.GetBuses().begin(), 
		base_.transport_catalogue.GetBuses().end() };

	RequestHandler request_handler(base_.transport_catalogue, map_renderer, base_.router);
	return StatRequests(stat_node, request_handler);
}

::json::Node StatReader::StatRequests(const Node& stat_node, const RequestHandler& request_handler) {
	Array result{};
	for (auto& request : stat_node.AsArray()) {
		result.push_back(StatRequest(request.AsDict(), request_handler));
	}
	return ::json::Node{ move(result) };
}

::json::Node StatReader::StatRequest(const Dict& request, const RequestHandler& request_handler) {
	auto& type = request.at("type");
	if (type == "Bus"s) {
		return BusRequest(request, request_handler);
	}

	if (type == "Stop"s) {
		return StopRequest(request, request_handler);
	}

	if (type == "Map"s) {
		return MapRequest(request, request_handler);
	}

	if (type == "Route"s) {
		return RouteRequest(request, request_handler);
	}
	return ::json::Node{ Dict {} };
}

Node StatReader::BusRequest(const Dict& request, const RequestHandler& request_handler) {
	auto builder = Builder{}
		.StartDict()
		.Key("request_id").Value(request.at("id").AsInt());

	auto request_result = request_handler.GetBusStat(request.at("name").AsString());

	if (!request_result) {
		return builder.Key("error_message").Value("not found"s).EndDict().Build();
	}

	return builder
		.Key("curvature").Value(request_result->curvature)
		.Key("route_length").Value(static_cast<int>(request_result->route_length))
		.Key("stop_count").Value(static_cast<int>(request_result->stop_count))
		.Key("unique_stop_count").Value(static_cast<int>(request_result->unique_stop_count))
		.EndDict().Build();
}


::json::Node StatReader::StopRequest(const Dict& request, const RequestHandler& request_handler) {
	auto builder = Builder{}
		.StartDict()
		.Key("request_id").Value(request.at("id").AsInt());

	auto request_result = request_handler.GetSortedBusesByStop(request.at("name").AsString());

	if (!request_result) {
		return builder.Key("error_message").Value("not found"s).EndDict().Build();
	}

	auto array_builder = builder.Key("buses").StartArray();

	for (auto& name : *request_result) {
		array_builder.Value(std::string(name));
	}

	return array_builder.EndArray().EndDict().Build();
}


::json::Node StatReader::MapRequest(const Dict& request, const RequestHandler& request_handler) {
	return Builder{}
		.StartDict()
		.Key("request_id").Value(request.at("id").AsInt())
		.Key("map").Value(request_handler.RenderMap())
		.EndDict()
		.Build();
}


::json::Node StatReader::RouteRequest(const Dict& request, const RequestHandler& request_handler) {
	using namespace std::string_literals;
	auto builder = Builder{}
		.StartDict()
		.Key("request_id").Value(request.at("id").AsInt());

	auto route = request_handler.BuildRoute(request.at("from").AsString(), request.at("to").AsString());

	if (!route) {
		return builder
			.Key("error_message")
			.Value("not found")
			.EndDict()
			.Build();
	}

	const Router::RouteInfo& route_val = route.value();
	auto items = builder
		.Key("total_time")
		.Value(route_val.total_time)
		.Key("items")
		.StartArray();
	auto& stops = request_handler.GetTransportCatalogue().GetStops();
	auto& buses = request_handler.GetTransportCatalogue().GetBuses();
	for (auto& item : route_val.events) {
		if (const router::Span* pval = std::get_if<router::Span>(&item)) {

			auto node = Builder{}
				.StartDict()
				.Key("bus").Value(buses[pval->bus].name_)
				.Key("span_count").Value(static_cast<int>(pval->count))
				.Key("time").Value(pval->time)
				.Key("type").Value("Bus"s)
				.EndDict()
				.Build();

			items.Value(node
				.AsDict());
		}
		else if (const router::Wait* pval = std::get_if<router::Wait>(&item)) {
			auto node = Builder{}
				.StartDict()
				.Key("stop_name").Value(stops[pval->stop].name_)
				.Key("time").Value(pval->time)
				.Key("type").Value("Wait"s)
				.EndDict()
				.Build();

			items.Value(node
				.AsDict());
		}
	}
	return items
		.EndArray()
		.EndDict()
		.Build();
}


} //namespace json
} //namespace transport