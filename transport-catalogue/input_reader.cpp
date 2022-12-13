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

static pair<string, size_t> ParsingName(const string& body, size_t pos = 0) {
	size_t name_end = body.find(':', pos);
	auto ret = body.substr(1, name_end - 1);
	++name_end;
	return {move(ret), name_end };
}

static pair<geo::Coordinates, size_t> ParsingCoordinates(const string& body, size_t pos = 0) {
	size_t end_lat = body.find(',', pos);
	geo::Coordinates coordinates;
	coordinates.lat = stod(body.substr(pos, end_lat - pos));
	++end_lat;
	size_t end_lon = body.find(',', end_lat);

	coordinates.lng = (end_lon == string::npos) ? stod(body.substr(end_lat)) : stod(body.substr(end_lat, end_lon - end_lat));
	return { coordinates, end_lon };
}

static void ParsingStop(istream& is, TransportCatalogue& transport_catalogue, unordered_map<string, unordered_map<string, size_t>>& length_from_to) {
	
	string temp;
	getline(is, temp);

	auto name_pos = ParsingName(temp);
	auto coordinates_pos = ParsingCoordinates(temp, name_pos.second);

	size_t& end_lon = coordinates_pos.second;
    if(end_lon != string::npos) {
        end_lon += 2;

        size_t length = end_lon;
        size_t length_end;
        for(;;) {
            length_end = temp.find(',', length);
            if(length_end == string::npos) length_end = temp.size();
            size_t end_digit = temp.find("m to ", length);
            size_t len = stol(temp.substr(length, end_digit - length));
            end_digit += 5;
            length_from_to[name_pos.first][trim(temp.substr(end_digit, length_end - end_digit))] = len;
            if(length_end == temp.size()) {
                break;
            }
            length = length_end + 1;
        }
    }
	transport_catalogue.AddStop(move(name_pos.first), move(coordinates_pos.first));
}

static void ParsingBus(string& bus_string, TransportCatalogue& transport_catalogue) {
	auto name_pos = ParsingName(bus_string);
	char symbol = '>';
	auto first_stop_end = bus_string.find(symbol, name_pos.second);
	bool bus_circular;
	if (first_stop_end == string::npos) {
		symbol = '-';
		first_stop_end = bus_string.find(symbol, name_pos.second);
		bus_circular = false;
	} else {
		bus_circular = true;
	}

	list<string> stop_names;

	stop_names.push_back(trim(bus_string.substr(name_pos.second, first_stop_end - name_pos.second)));

	for (auto left = first_stop_end; left != string::npos;) {
		++left;
		auto end_of_stop = bus_string.find(symbol, left);
		stop_names.push_back(trim(bus_string.substr(left, end_of_stop == string::npos ? string::npos : end_of_stop - left)));
		left = end_of_stop;
	}

	transport_catalogue.AddBus(move(name_pos.first), bus_circular, stop_names);
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
			ParsingStop(is, transport_catalogue, length_from_to);
		} else {
			string temp;
			getline(is, temp);
			buses.push_back(move(temp));
		}
	}

	for (auto& bus: buses) {
		ParsingBus(bus, transport_catalogue);
	}

	transport_catalogue.SetLengthBetweenStops(length_from_to);
    
	return is;
}
