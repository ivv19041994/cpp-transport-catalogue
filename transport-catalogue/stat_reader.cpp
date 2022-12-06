#include "stat_reader.h"

#include <string>
#include <execution>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace transport;

static void s_BusReq(ostream& os, TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Bus " << name << ": ";
	try {
		const Bus& bus = transport_catalogue.GetBus(name);
		auto route_length = bus.GetLength();
		os << bus.GetStopsCount() << " stops on route, "
			<< bus.GetUniqueStopsCount() << " unique stops, "
			<< route_length << " route length, "
			<< setprecision(6) << (route_length / bus.GetGeoLength()) << " curvature" << endl;
	}
	catch (const out_of_range& e) {
		os << "not found" << endl;
	}
}

static void s_StopReq(ostream& os, TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Stop " << name << ": ";
	try {
		const Stop& stop = transport_catalogue.GetStop(name);
		auto buses = stop.GetBuses();
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
	catch (const out_of_range& e) {
		os << "not found" << endl;
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
			s_BusReq(os, transport_catalogue, body);
		} else if(req == "Stop") {
			s_StopReq(os, transport_catalogue, body);
        }
	}
}