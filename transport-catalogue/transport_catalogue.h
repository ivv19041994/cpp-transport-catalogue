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
#include <functional>
#include <iostream>



#include "domain.h"


namespace transport {

	

	template <typename Pointer>
	struct PairPointerHasher {
		size_t operator() (const std::pair<Pointer, Pointer>& f) const {
			static constexpr std::hash<Pointer> hasher;
			return hasher(f.first) + hasher(f.second) * 977;
		}
	};

	class TransportCatalogue {
	public:
		using length_map = std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, PairPointerHasher<const Stop*>>;
	
		template<typename Container>
		void AddBus(std::string name, bool circular, const Container& stop_names);
		const Bus* GetBus(const std::string_view bus_name) const;
		size_t GetStopsCount(const Bus* bus) const;
		size_t GetUniqueStopsCount(const Bus* bus) const;
		double GetGeoLength(const Bus* bus) const;
		size_t GetLength(const Bus* bus) const;
		const std::deque<Bus>& GetBuses() const;

		void AddStop(std::string name, geo::Coordinates coordinates);
		const Stop* GetStop(const std::string_view stop_name) const;
		std::set<std::string> GetBusesNamesFromStop(const Stop*) const;
		size_t GetLengthFromTo(const Stop* from, const Stop* to) const;
		const std::unordered_set<Bus*>* GetBusesByStop(const Stop*) const;

		void SetLengthBetweenStops(const std::unordered_map<std::string, std::unordered_map<std::string, size_t>>& length_from_to);

		const std::deque<Stop>& GetStops() const;
		
		const length_map& GetLengthMap() const;
		
	private:
		

		
		std::deque<Bus> buses_storage_;
		std::deque<Stop> stops_storage_;
		std::unordered_map<std::string_view, Stop*> stops_;
		std::unordered_map<std::string_view, Bus*> buses_;
		length_map length_from_to_;
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

