#pragma once

#include <optional>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "request_handler.h"


namespace transport {
namespace json {

using ::json::Node;
using ::json::Document;
using ::json::Array;
using ::json::Dict;
using ::transport::router::Router;
using ::transport::router::RouterSettings;
using ::transport::renderer::RenderSettings;

struct Base {
	transport::TransportCatalogue transport_catalogue;
	RenderSettings render_settings;
	Router router;
};

class BaseReader {
public:
	Base operator()(const Document& document);
private:
	transport::TransportCatalogue transport_catalogue_;
	RenderSettings render_settings_;
	RouterSettings router_settings_;
	std::optional<Router> router_;

	void InputReader(const Node& input_node);
	
};

class StatReader {
public:
	StatReader(const Base& base);
	Document operator()(const Document& document);
private:
	const Base& base_;

	Node StatRequests(const Node& stat_node);
	Node StatRequests(const Node& stat_node, const RequestHandler& request_handler);

	Node StatRequest(const Dict& request, const RequestHandler& request_handler);
	Node BusRequest(const Dict& request, const RequestHandler& request_handler);
	Node StopRequest(const Dict& request, const RequestHandler& request_handler);
	Node MapRequest(const Dict& request, const RequestHandler& request_handler);
	Node RouteRequest(const Dict& request, const RequestHandler& request_handler);
};
}
}