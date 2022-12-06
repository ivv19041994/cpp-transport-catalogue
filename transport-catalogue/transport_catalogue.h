#pragma once 

#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <list>
#include <set>


#include "geo.h"

namespace transport {
	class Stop;
	class Bus;
	class TransportCatalogue;

	class Bus {
	public:

		Bus(std::string name, bool circular);

		const std::string& GetName() const;
		void AddStop(Stop& stop);//возможно стоит спрятать в private и дать доступ только через TransportCatalogue (к тому же он уже и так friend)
		size_t GetStopsCount() const;
		size_t GetUniqueStopsCount() const;
		double GetGeoLength() const;
		size_t GetLength() const;

	private:
		friend class TransportCatalogue;
		std::string name_;
		bool circular_;
		std::list<Stop*> stops_;
		std::unordered_set<std::string_view> stops_names_;

		
	};

	class Stop {
	public:
		Stop(std::string name, double latitude, double longitude);
		const std::string& GetName() const;
		double GetLatitude() const;
		double GetLongitude() const;
		geo::Coordinates GetCoordinates() const;

		void AddLengthTo(const Stop& other, size_t length);
		void AddIfNotSetLengthTo(const Stop& other, size_t length);
		size_t GetLengthTo(const Stop& other) const;

		std::set<std::string> GetBuses() const;
		void AddBus(Bus& bus);//возможно стоит спрятать в private и дать доступ только через TransportCatalogue

	private:
		std::string name_;
		geo::Coordinates coordinates_;
		std::list<Bus*> buses_;
		std::unordered_map<const Stop*, size_t> length_to_;
	};

	class TransportCatalogue {
	public:
		void AddBus(Bus bus);
		void AddStopToBus(const std::string_view& stop, const std::string_view& bus);
		const Bus& GetBus(const std::string_view bus_name) const;
		//Route& GetRoute(const std::string_view route_name);

		void AddStop(Stop stop);
		const Stop& GetStop(const std::string_view stop_name) const;
		Stop& GetStop(const std::string_view stop_name);
		void SetLengthBetweenStops(Stop& from, Stop& to, size_t length);
		//std::shared_ptr<Stop> GetStopPtr(const std::string_view stop_name);


	private:
		std::unordered_map<std::string_view, std::shared_ptr<Stop>> stops_;
		std::unordered_map<std::string_view, std::shared_ptr<Bus>> buses_;
	};
}

