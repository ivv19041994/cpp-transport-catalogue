#pragma once 

#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <list>
#include <set>
#include <map>


#include "geo.h"

namespace transport {
	struct Stop;
	struct Bus;
	class TransportCatalogue;

	using ConteinerOfStopPointers = std::list<Stop*>;

	struct Bus {
		std::string name_;
		bool circular_;
		ConteinerOfStopPointers stops_;
		std::unordered_set<Stop*> stops_set_;
	};

	struct Stop {
		std::string name_;
		geo::Coordinates coordinates_;
		std::unordered_set<Bus*> buses_;
	};

	template <typename Pointer>
	struct PairPointerHasher {
		size_t operator() (const std::pair<Pointer, Pointer>& f) const {
			return reinterpret_cast<size_t>(f.first)  + reinterpret_cast<size_t>(f.second) * 977;
		}
	};

	class TransportCatalogue {
	public:
		template<typename It>
		void AddBus(std::string name, bool circular, It begin_stops, It end_stops);
		const Bus* GetBus(const std::string_view bus_name) const;
		size_t GetStopsCount(const Bus* bus) const;
		size_t GetUniqueStopsCount(const Bus* bus) const;
		double GetGeoLength(const Bus* bus) const;
		size_t GetLength(const Bus* bus) const;

		void AddStop(Stop stop);
		const Stop* GetStop(const std::string_view stop_name) const;
		std::set<std::string> GetBusesNamesFromStop(const Stop*) const;
		size_t GetLengthFromTo(const Stop* from, const Stop* to) const;


		void SetLengthBetweenStops(const std::unordered_map<std::string, std::unordered_map<std::string, size_t>>& length_from_to);


	private:
		std::unordered_map<std::string_view, std::shared_ptr<Stop>> stops_;
		std::unordered_map<std::string_view, std::shared_ptr<Bus>> buses_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, PairPointerHasher<const Stop*>> length_from_to_;
	};

	template<typename It>
	void TransportCatalogue::AddBus(std::string name, bool circular, It begin_stops, It end_stops) {
		ConteinerOfStopPointers stops;

		for (; begin_stops != end_stops; ++begin_stops) {
			stops.push_back(stops_[*begin_stops].get());
		}

		std::unordered_set<Stop*> stops_set{ stops.begin(), stops.end()};
        
        Bus bus{
            std::move(name), circular, std::move(stops), std::move(stops_set)
        };

		std::shared_ptr<Bus> spbus = std::make_shared<Bus>(std::move(bus));

		buses_[spbus->name_] = spbus;
		Bus* pbus = spbus.get();
		for (Stop* pstop : spbus->stops_set_) {
			pstop->buses_.insert(pbus);
		}
	}
}

