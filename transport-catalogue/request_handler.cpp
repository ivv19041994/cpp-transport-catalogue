#include "request_handler.h"

namespace transport {

RequestHandler::RequestHandler(const TransportCatalogue& db/*, const renderer::MapRender& renderer*/) : db_{ db } {
}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	BusStat ret;

	const Bus* bus = db_.GetBus(bus_name);
	if (!bus) {
		return std::optional<BusStat>{};
	}

	ret.route_length = db_.GetLength(bus);
	ret.curvature = ret.route_length / db_.GetGeoLength(bus);
	ret.stop_count = db_.GetStopsCount(bus);
	ret.unique_stop_count = db_.GetUniqueStopsCount(bus);
	return std::optional<BusStat>{std::move(ret)};
}

// Возвращает маршруты, проходящие через
const std::unordered_set<BusPtr>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {

	const Stop* pstop = db_.GetStop(stop_name);

	if (!pstop) {
		return nullptr;
	}

	return db_.GetBusesByStop(pstop);
}

const std::optional <std::set<std::string_view>> RequestHandler::GetSortedBusesByStop(const std::string_view& stop_name) const {
	auto buses = GetBusesByStop(stop_name);
	std::optional <std::set<std::string_view>> ret;

	if (!buses) {
		return ret;
	}
	std::set<std::string_view> ret_set;
	for (auto pbus : *buses) {
		ret_set.insert(pbus->name_);
	}
	ret = std::move(ret_set);
	return ret;
}

}//namespace transport