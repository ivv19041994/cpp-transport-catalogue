#pragma once 

#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <list>
#include <set>
#include <map>
#include <deque>


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
		//std::unordered_set<Bus*> buses_;
	};

	template <typename Pointer>
	struct PairPointerHasher {
		size_t operator() (const std::pair<Pointer, Pointer>& f) const {
			static constexpr hash<Pointer> hasher;
			return hasher(f.first) + hasher(f.second) * 977;
		}
	};

	class TransportCatalogue {
	public:
		template<typename Container>
		void AddBus(std::string name, bool circular, const Container& stop_names);
		const Bus* GetBus(const std::string_view bus_name) const;
		size_t GetStopsCount(const Bus* bus) const;
		size_t GetUniqueStopsCount(const Bus* bus) const;
		double GetGeoLength(const Bus* bus) const;
		size_t GetLength(const Bus* bus) const;

		void AddStop(std::string name, geo::Coordinates coordinates);
		const Stop* GetStop(const std::string_view stop_name) const;
		std::set<std::string> GetBusesNamesFromStop(const Stop*) const;
		size_t GetLengthFromTo(const Stop* from, const Stop* to) const;


		void SetLengthBetweenStops(const std::unordered_map<std::string, std::unordered_map<std::string, size_t>>& length_from_to);


	private:
		std::deque<Bus> buses_storage_;
		std::deque<Stop> stops_storage_;
		std::unordered_map<std::string_view, Stop*> stops_;
		std::unordered_map<std::string_view, Bus*> buses_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, PairPointerHasher<const Stop*>> length_from_to_;
		std::unordered_map<const Stop*, std::unordered_set<Bus*>> buses_of_stop_;
	};

	template<typename Container>
	void TransportCatalogue::AddBus(std::string name, bool circular, const Container& stop_names) {
		ConteinerOfStopPointers stops;

		for (auto& name : stop_names) {
			stops.push_back(stops_[name]);
		}

		std::unordered_set<Stop*> stops_set{ stops.begin(), stops.end() };

		Bus bus{
			std::move(name), circular, std::move(stops), std::move(stops_set)
		};

		buses_storage_.push_back(std::move(bus));
		Bus* pbus = &buses_storage_.back();

		//std::shared_ptr<Bus> spbus = std::make_shared<Bus>(std::move(bus));

		buses_[pbus->name_] = pbus;
		//Bus* pbus = spbus.get();
		for (Stop* pstop : pbus->stops_set_) {
			buses_of_stop_[pstop].insert(pbus);
		}
	}


}

