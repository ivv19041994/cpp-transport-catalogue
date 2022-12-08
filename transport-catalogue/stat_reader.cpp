#include "stat_reader.h"

#include <string>
#include <execution>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace transport;

static void BusRequest(ostream& os, TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Bus " << name << ": ";

	const Bus* bus = transport_catalogue.GetBus(name);
	if (!bus) {
		os << "not found" << endl;
		return;
	}
	auto route_length = transport_catalogue.GetLength(bus);
	os << transport_catalogue.GetStopsCount(bus) << " stops on route, "
		<< transport_catalogue.GetUniqueStopsCount(bus) << " unique stops, "
		<< route_length << " route length, "
		<< setprecision(6) << (route_length / transport_catalogue.GetGeoLength(bus)) << " curvature" << endl;
}

static void StopRequest(ostream& os, TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Stop " << name << ": ";

	const Stop* stop = transport_catalogue.GetStop(name);
	if (!stop) {
		os << "not found" << endl;
		return;
	}
	auto buses = transport_catalogue.GetBusesNamesFromStop(stop); //stop->buses_;
	if (buses.size()) {
		os << "buses";
		for (auto& name : buses) {
			os << " " << name;
		}
		os << endl;
	}
	else {
		os << "no buses" << endl;
	}

}

void transport::iostream::StatReader(istream& is, ostream& os, TransportCatalogue& transport_catalogue) {
	int n;
	is >> n;
	
	for (int i = 0; i < n; ++i) {
		string req;
		is >> req;
		string body;
		getline(is, body);
		if (req == "Bus") {
			BusRequest(os, transport_catalogue, body);
		} else if(req == "Stop") {
			StopRequest(os, transport_catalogue, body);
        }
	}
}