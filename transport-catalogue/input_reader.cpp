#include "input_reader.h"

#include <vector>
#include <string>

using namespace std;
using namespace transport;

#include <iostream>
#include <iomanip>

static string trim(const std::string& s) {
	const std::string trimmed = " \n\r\t\f\v"s;
	size_t start = s.find_first_not_of(trimmed);
	if (start == std::string::npos) {
		return ""s;
	}
	size_t end = s.find_last_not_of(trimmed);

	return s.substr(start, end - start + 1);
}

static void s_addStop(istream& is, TransportCatalogue& transport_catalogue, unordered_map<string, unordered_map<string, size_t>>& length_from_to) {
	
	string temp;
	getline(is, temp);

	size_t name_end = temp.find(':');
	auto stop_name = temp.substr(1, name_end - 1);
	++name_end;
	size_t end_lat = temp.find(',', name_end);
	auto stop_latitude = stod(temp.substr(name_end, end_lat - name_end));
    ++end_lat;
    size_t end_lon = temp.find(',', end_lat);
    
	auto stop_longitude = (end_lon == string::npos) ? stod(temp.substr(end_lat)) : stod(temp.substr(end_lat, end_lon - end_lat));
    
    if(end_lon != string::npos) {
        end_lon += 2;

        size_t length = end_lon;
        size_t length_end;
        for(;;) {

            //cout << "for(;;) " << temp.substr(length) << endl;

            length_end = temp.find(',', length);
            if(length_end == string::npos) length_end = temp.size();
            size_t end_digit = temp.find("m to ", length);
            //cout << "stol " << temp.substr(length, end_digit - length) << endl;
            size_t len = stol(temp.substr(length, end_digit - length));
            end_digit += 5;
            //cout << "length_from_to[" << stop_name << "][" << trim(temp.substr(end_digit, length_end - end_digit)) << "] = " << len << endl;
            length_from_to[stop_name][trim(temp.substr(end_digit, length_end - end_digit))] = len;
            if(length_end == temp.size()) {
                break;
            }
            length = length_end + 1;
        }
    }
	//cout << stop.name_ << " " << setprecision(8) << stop.latitude_ << " " << setprecision(8) << stop.longitude_ << endl;
	transport_catalogue.AddStop(Stop{move(stop_name), stop_latitude, stop_longitude});
}

static void s_addBus(string& bus_string, TransportCatalogue& transport_catalogue) {
	auto name_end = bus_string.find(':');
	string bus_name = bus_string.substr(1, name_end - 1);
	++name_end;
	char symbol = '>';
	auto first_stop_end = bus_string.find(symbol, name_end);
	bool bus_circular;
	if (first_stop_end == string::npos) {
		symbol = '-';
		first_stop_end = bus_string.find(symbol, name_end);
		bus_circular = false;
	} else {
		bus_circular = true;
	}

	Bus bus(move(bus_name), bus_circular);

	bus.AddStop(transport_catalogue.GetStop(trim(bus_string.substr(name_end, first_stop_end - name_end))));

	for (auto left = first_stop_end; left != string::npos;) {
		++left;
		auto end_of_stop = bus_string.find(symbol, left);
		bus.AddStop(transport_catalogue.GetStop(trim(bus_string.substr(left, end_of_stop == string::npos ? string::npos : end_of_stop - left))));
		left = end_of_stop;
	}

	transport_catalogue.AddBus(move(bus));
}

istream& transport::iostream::InputReader(istream& is, TransportCatalogue& transport_catalogue) {
	int n;
	is >> n;
	string type;
	getline(is, type);
	vector<string> buses;
	buses.reserve(n);
    unordered_map<string, unordered_map<string, size_t>> length_from_to;
	for (int i = 0; i < n; ++i) {
		is >> type;

		if (type == "Stop") {
			s_addStop(is, transport_catalogue, length_from_to);
		} else {
			string temp;
			getline(is, temp);
			buses.push_back(move(temp));
		}
	}

	for (auto& bus: buses) {
		s_addBus(bus, transport_catalogue);
	}
    
    for(auto& [from, val] : length_from_to) {
        auto& from_stop = transport_catalogue.GetStop(from);
        for(auto& [to, len]: val) {
            //cout << "from " << from << " to " << to << " = " << len << endl;
            transport_catalogue.SetLengthBetweenStops(from_stop, transport_catalogue.GetStop(to), len);
        }
    }


	return is;
}
