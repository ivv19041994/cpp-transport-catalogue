#include "request_handler.h"
#include <map>
#include <unordered_map>

namespace transport {

RequestHandler::RequestHandler(const TransportCatalogue& db, const renderer::MapRender& renderer, const router::Router& router)
	: db_{ db } 
	, renderer_{ renderer }
	, router_{ router } {
}




// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat>& RequestHandler::GetBusStat(const std::string_view bus_name) const {
	

	auto it = bus_stat_cash_.find(bus_name);
	if (it != bus_stat_cash_.end()) {
		return it->second;
	}

	const Bus* bus = db_.GetBus(bus_name);
	if (!bus) {
		return bus_stat_cash_[bus_name] = std::optional<BusStat>{};
	}

	BusStat ret;
	ret.route_length = db_.GetLength(bus);
	ret.curvature = ret.route_length / db_.GetGeoLength(bus);
	ret.stop_count = db_.GetStopsCount(bus);
	ret.unique_stop_count = db_.GetUniqueStopsCount(bus);

	return bus_stat_cash_[bus_name] = std::move(ret);
}

// Возвращает маршруты, проходящие через
const std::unordered_set<BusPtr>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {

	const Stop* pstop = db_.GetStop(stop_name);

	if (!pstop) {
		return nullptr;
	}

	return db_.GetBusesByStop(pstop);
}

const std::optional <std::set<std::string_view>>& RequestHandler::GetSortedBusesByStop(const std::string_view stop_name) const {
	
	auto it = stop_stat_cash_.find(stop_name);
	if (it != stop_stat_cash_.end()) {
		return it->second;
	}
	
	auto buses = GetBusesByStop(stop_name);
	

	if (!buses) {
		return stop_stat_cash_[stop_name];
	}

	std::set<std::string_view> ret_set;
	for (auto pbus : *buses) {
		ret_set.insert(pbus->name_);
	}
	return stop_stat_cash_[stop_name] = std::move(ret_set);
}

std::optional<Router::RouteInfo> RequestHandler::BuildRoute(const std::string_view from, const std::string_view to) const {
	const Stop* stop_from = db_.GetStop(from);
	const Stop* stop_to = db_.GetStop(to);

	if (stop_from == nullptr || stop_to == nullptr) {
		return {};
	}

	return router_.BuildRoute(stop_from, stop_to);
}

std::string RequestHandler::RenderMap() const {
	return renderer_.Render();
}

}//namespace transport