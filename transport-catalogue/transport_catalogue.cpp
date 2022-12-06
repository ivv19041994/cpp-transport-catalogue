#include "transport_catalogue.h"

#include <numeric>
#include <stdexcept>


#include "geo.h"




using namespace std;

namespace transport {
	void TransportCatalogue::AddStop(Stop stop) {
		shared_ptr pstop = make_shared<Stop>(move(stop));
		string_view name = pstop->GetName();
		stops_.try_emplace(name, pstop);
	}

	void TransportCatalogue::AddBus(Bus bus) {
		shared_ptr pbus = make_shared<Bus>(move(bus));
		string_view name = pbus->GetName();
		buses_.try_emplace(name, pbus);
		for (auto& stop : pbus->stops_) {
			stop->AddBus(*pbus);
		}
	}
	
	void TransportCatalogue::AddStopToBus(const string_view& stop, const string_view& bus) {
		buses_[bus]->AddStop(*stops_[stop]);
	}

	Bus::Bus(std::string name, bool circular) : name_{ move(name) }, circular_{ circular } {

	}

	void Bus::AddStop(Stop& stop) {
		stops_.push_back(&stop);
		stops_names_.insert(stop.GetName());
	}

	void Stop::AddBus(Bus& bus) {
		buses_.push_back(&bus);
	}

	set<string> Stop::GetBuses() const {
		set<string> ret;
		for (auto bus : buses_) {
			ret.insert(bus->GetName());
		}
		return ret;
	}

	const std::string& Bus::GetName() const {
		return name_;
	}

	size_t Bus::GetStopsCount() const {
		return circular_ ? stops_.size() : stops_.size() * 2 - 1;
	}

	size_t Bus::GetUniqueStopsCount() const {
		return stops_names_.size();
	}

	double Bus::GetGeoLength() const {
		double res = transform_reduce(
			++stops_.begin(), stops_.end(),
			stops_.begin(),
			0.0,
			plus<>(),
			[](const auto& lhs, const auto& rhs) {
				return ComputeDistance(lhs->GetCoordinates(), rhs->GetCoordinates());
			}
		);

		return circular_ ? res : res * 2;
	}

	size_t Bus::GetLength() const {
		size_t res = 0;

		auto get_length_to = [](const auto& lhs, const auto& rhs) {
			return rhs->GetLengthTo(*lhs);
		};

		res = transform_reduce(
			++stops_.begin(), stops_.end(),
			stops_.begin(),
			res,
			plus<>(),
			get_length_to
		);

		Stop& first = *stops_.front();
		res += first.GetLengthTo(first);
		if (circular_) {
			return res;
		}

		Stop& last = *stops_.back();
		res += last.GetLengthTo(last);

		res = transform_reduce(
			++stops_.rbegin(), stops_.rend(),
			stops_.rbegin(),
			res,
			plus<>(),
			get_length_to
		);


		return res;
	}

	Stop::Stop(std::string name, double latitude, double longitude) :
		name_{ move(name) }, coordinates_{ latitude, longitude } {
	}

	const std::string& Stop::GetName() const {
		return name_;
	}

	double Stop::GetLatitude() const {
		return coordinates_.lat;
	}

	double Stop::GetLongitude() const {
		return coordinates_.lng;
	}

	geo::Coordinates Stop::GetCoordinates() const {
		return coordinates_;
	}

	const Bus& TransportCatalogue::GetBus(const std::string_view route_name) const {
		auto at = buses_.at(route_name);
		return *at;
	}

	const Stop& TransportCatalogue::GetStop(const std::string_view stop_name) const {
		auto at = stops_.at(stop_name);
		return *at;
	}

	Stop& TransportCatalogue::GetStop(const std::string_view stop_name) {
		auto at = stops_.at(stop_name);
		return *at;
	}

	void Stop::AddLengthTo(const Stop& other, size_t length) {
		length_to_[&other] = length;
	}

	void Stop::AddIfNotSetLengthTo(const Stop& other, size_t length) {
		try {
			length_to_.at(&other);
		}
		catch (const out_of_range&) {
			length_to_[&other] = length;
		}
	}

	size_t Stop::GetLengthTo(const Stop& other) const {
		try {
			return length_to_.at(&other);
		}
		catch (const out_of_range& e) {
			return 0;
		}
	}

	void TransportCatalogue::SetLengthBetweenStops(Stop& from, Stop& to, size_t length) {
		from.AddLengthTo(to, length);
		to.AddIfNotSetLengthTo(from, length);
	}
}



