#include "transport_catalogue.h"

#include <numeric>
#include <stdexcept>
using namespace std;

namespace transport {

	const std::unordered_set<Bus*>* TransportCatalogue::GetBusesByStop(const Stop* pstop) const {
		return &buses_of_stop_.at(pstop);
	}

	void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
        
		stops_storage_.push_back(Stop{move(name), move(coordinates)});
		Stop* pstop = &stops_storage_.back();
		stops_[pstop->name_] = pstop;
		buses_of_stop_[pstop];
	}

	set<string> TransportCatalogue::GetBusesNamesFromStop(const Stop* stop) const {
		set<string> ret;

		for (auto bus : buses_of_stop_.at(stop)) {
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
			ComputeDistance
		);

		return bus->circular_ ? res : res * 2;
	}

	size_t TransportCatalogue::GetLengthFromTo(const Stop* from, const Stop* to) const {
		//try {
			auto it = length_from_to_.find({ from, to }); 
			if(it == length_from_to_.end()) {
				it = length_from_to_.find({ to, from });
				if(it == length_from_to_.end()) {
                    return 0;
                }
			}
			return it->second;
			
			//return length_from_to_.at({ from, to });
		//}
		//catch (const out_of_range& e) {
		//	return 0;
		//}
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

		return buses_.at(route_name);
	}

	const Stop* TransportCatalogue::GetStop(const std::string_view stop_name) const {
		if(stops_.count(stop_name) == 0) {
            return nullptr;
        }

		return stops_.at(stop_name);
	}

	void TransportCatalogue::SetLengthBetweenStops(const std::unordered_map<std::string, std::unordered_map<std::string, size_t>>& length_from_to) {
		
		for (auto& [from, to_map] : length_from_to) {
			Stop* pfrom = stops_[from];
			for (auto& [to, length] : to_map) {
				Stop* pto = stops_[to];
				length_from_to_[{pfrom, pto}] = length;
				pair pto_pfrom = { pto, pfrom };
				if (length_from_to_.count(pto_pfrom) == 0) {
					length_from_to_[pto_pfrom] = length;
				}
			}
		}
	}

	const std::deque<Stop>& TransportCatalogue::GetStops() const {
		return stops_storage_;
	}

	const std::deque<Bus>& TransportCatalogue::GetBuses() const {
		return buses_storage_;
	}
	
	const TransportCatalogue::length_map& TransportCatalogue::GetLengthMap() const {
		return length_from_to_;
	}
}



