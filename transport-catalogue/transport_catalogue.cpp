#include "transport_catalogue.h"

#include <numeric>
#include <stdexcept>


#include "geo.h"




using namespace std;

namespace transport {

	void TransportCatalogue::AddStop(Stop stop) {
		auto pstop = make_shared<Stop>(move(stop));
		stops_[pstop->name_] = pstop;
	}

	set<string> TransportCatalogue::GetBusesNamesFromStop(const Stop* stop) const {
		set<string> ret;

		for (auto bus : stop->buses_) {
			ret.insert(bus->name_);
		}
		return ret;
	}

	size_t TransportCatalogue::GetStopsCount(const Bus *bus) const {
		return bus->circular_ ? bus->stops_.size() : bus->stops_.size() * 2 - 1;
	}

	size_t TransportCatalogue::GetUniqueStopsCount(const Bus* bus) const {
		return bus->stops_set_.size();
	}

	double TransportCatalogue::GetGeoLength(const Bus* bus) const {
		double res = transform_reduce(
			++bus->stops_.begin(), bus->stops_.end(),
			bus->stops_.begin(),
			0.0,
			plus<>(),
			[](const auto& lhs, const auto& rhs) {
				return ComputeDistance(lhs->coordinates_, rhs->coordinates_);
			}
		);

		return bus->circular_ ? res : res * 2;
	}

	size_t TransportCatalogue::GetLengthFromTo(const Stop* from, const Stop* to) const {
		try {
			return length_from_to_.at({ from, to });
		}
		catch (const out_of_range& e) {
			return 0;
		}
	}

	size_t TransportCatalogue::GetLength(const Bus* bus) const {
		size_t res = 0;

		auto get_length_to = [this](const auto& lhs, const auto& rhs) {
			return this->GetLengthFromTo(rhs, lhs);
		};

		res = transform_reduce(
			++bus->stops_.begin(), bus->stops_.end(),
			bus->stops_.begin(),
			res,
			plus<>(),
			get_length_to
		);

		Stop* first = bus->stops_.front();
		res += GetLengthFromTo(first, first);
		if (bus->circular_) {
			return res;
		}

		Stop* last = bus->stops_.back();
		res += GetLengthFromTo(last, last);

		res = transform_reduce(
			++bus->stops_.rbegin(), bus->stops_.rend(),
			bus->stops_.rbegin(),
			res,
			plus<>(),
			get_length_to
		);

		return res;
	}

	const Bus* TransportCatalogue::GetBus(const std::string_view route_name) const {
		if(buses_.count(route_name) == 0) {
            return nullptr;
        }

		return buses_.at(route_name).get();
	}

	const Stop* TransportCatalogue::GetStop(const std::string_view stop_name) const {
		if(stops_.count(stop_name) == 0) {
            return nullptr;
        }

		return stops_.at(stop_name).get();
	}

	void TransportCatalogue::SetLengthBetweenStops(const std::unordered_map<std::string, std::unordered_map<std::string, size_t>>& length_from_to) {
		
		for (auto& [from, to_map] : length_from_to) {
			Stop* pfrom = stops_[from].get();
			for (auto& [to, length] : to_map) {
				Stop* pto = stops_[to].get();
				length_from_to_[{pfrom, pto}] = length;
				pair pto_pfrom = { pto, pfrom };
				if (length_from_to_.count(pto_pfrom) == 0) {
					length_from_to_[pto_pfrom] = length;
				}
			}
		}
	}
}



