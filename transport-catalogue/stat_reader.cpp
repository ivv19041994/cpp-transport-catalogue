#include "stat_reader.h"

#include <string>
#include <execution>
#include <iostream>
#include <iomanip>

#include "request_handler.h"

using namespace std;
using namespace transport;

static void BusRequest(ostream& os, const TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Bus " << name << ": ";

	RequestHandler request_handler{ transport_catalogue };
	auto request_result = request_handler.GetBusStat(name);
	//const Bus* bus = transport_catalogue.GetBus(name);
	if (!request_result) {
		os << "not found" << endl;
		return;
	}
	//auto route_length = transport_catalogue.GetLength(bus);
	os << request_result->stop_count << " stops on route, "
		<< request_result->unique_stop_count << " unique stops, "
		<< request_result->route_length << " route length, "
		<< setprecision(6) << request_result->curvature << " curvature" << endl;
}

static void StopRequest(ostream& os, const TransportCatalogue& transport_catalogue, const string& body) {
	string_view name = body.c_str() + 1;
	os << "Stop " << name << ": ";

	RequestHandler request_handler{ transport_catalogue };
	auto request_result = request_handler.GetSortedBusesByStop(name);

	//const Stop* stop = transport_catalogue.GetStop(name);
	if (!request_result) {
		os << "not found" << endl;
		return;
	}
	//auto buses = transport_catalogue.GetBusesNamesFromStop(stop); //stop->buses_;
	if (request_result->size()) {
		os << "buses";
		for (auto& name : *request_result) {
			os << " " << name;
		}
		os << endl;
	}
	else {
		os << "no buses" << endl;
	}

}

void transport::iostream::StatReader(istream& is, ostream& os, const TransportCatalogue& transport_catalogue) {
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