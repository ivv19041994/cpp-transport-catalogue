#include "serialization.h"
#include <cstdint>



#include <iostream>

namespace transport {
namespace serialize {
	
void DeserializeBase(transport::TransportCatalogue& transport_catalogue, const Base& base) {
	
	std::unordered_map<std::string, std::unordered_map<std::string, size_t>> length_from_to;
	for(int i = 0; i < base.stop_size(); ++i) {
		const Stop& stop = base.stop(i);
		transport_catalogue.AddStop(
			stop.name(),
			geo::Coordinates{ 
				stop.latitude(), 
				stop.longitude() 
			}
		);
		
		for(int length_ind = 0; length_ind < stop.road_distance_size(); ++length_ind) {
			uint32_t dis = stop.road_distance(length_ind);
			int other_id = dis & 0x7FF;
			const Stop& other = base.stop(other_id);
			length_from_to[stop.name()][other.name()] = dis >> 11;
		}
	}
	
	transport_catalogue.SetLengthBetweenStops(length_from_to);
	
	for(int bus_id = 0; bus_id < base.bus_size(); ++bus_id) {
		const Bus& bus = base.bus(bus_id);
		
		std::vector<std::string> stop_names;
		stop_names.reserve(bus.stop_id_size());
		
		for(int stop_index = 0; stop_index < bus.stop_id_size(); ++stop_index) {
			auto stop_id = bus.stop_id(stop_index);
			const Stop& stop = base.stop(stop_id);
			stop_names.push_back(stop.name());
		}
		/*std::cout << "AddBus |" << bus.name() << "|" <<  std::endl;
		std::cout << "is_roundtrip = " << bus.is_roundtrip() << std::endl;
		std::cout << "stop_names : " << std::endl;
		for(auto&n : stop_names) {
			std::cout << n << std::endl;
		}*/
		transport_catalogue.AddBus(bus.name(), bus.is_roundtrip(), stop_names);
	}
	
	/*for(const transport::Bus& bus: transport_catalogue.GetBuses()) {
		std::cout << "AddBus |" << bus.name_ << "|" <<  std::endl;
		std::cout << "is_roundtrip = " << bus.circular_ << std::endl;
		std::cout << "stop_names : " << std::endl;
		for(auto&n : bus.stops_) {
			std::cout << n->name_ << std::endl;
		}
	}*/
	
}
	
transport::TransportCatalogue DeserializeTransportCatalogue(std::istream& input) {
	transport::TransportCatalogue transport_catalogue;
	transport::serialize::TransportCatalogue load;
	
	if (!load.ParseFromIstream(&input)) {
        return transport_catalogue;
    }
	
	if(!load.has_base()) {
		return transport_catalogue;
	}
	
	DeserializeBase(transport_catalogue, load.base());
	return transport_catalogue;
}

Stop Create(const transport::Stop stop) {
	Stop ret;
	ret.set_latitude(stop.coordinates_.lat);
	ret.set_longitude(stop.coordinates_.lng);
	ret.set_name(stop.name_);
	return ret;
}

size_t GetStopIndex(const std::deque<transport::Stop>& stops, const transport::Stop* stop) {
	size_t ret = 0;
	for(const auto& s: stops) {
		if(&s == stop) {
			return ret;
		}
		++ret;
	}
	assert(false);
	return ret;
}

Base SerializeBase(const transport::TransportCatalogue& transport_catalogue) {
	Base base;
	
	const std::deque<transport::Stop>& stops = transport_catalogue.GetStops();
	for(auto& stop: stops) {
		*base.add_stop() = Create(stop);
	}
	const auto& length_map = transport_catalogue.GetLengthMap();
	
	//size_t cnt = 0;
	std::unordered_map<const transport::Stop*, std::unordered_map<const transport::Stop*, size_t>> uniq_len;
	for(const auto&[from_to, len]: length_map) {
		const transport::Stop* from = from_to.first;
		const transport::Stop* to = from_to.second;
		auto it_to = uniq_len.find(to);
		if(it_to != uniq_len.end()) {
			auto it_from = it_to->second.find(from);
			if(it_from != it_to->second.end()) {
				if(it_from->second == len) {
					continue;
				}
			}
		}
		uniq_len[from][to] = len;
		//++cnt;
	}
	
	//std::cout << "uniq_len =" << cnt << " length_map = " << length_map.size() << std::endl;;
	
	for(const auto&[pfrom, umap]: uniq_len) {
		size_t from = GetStopIndex(stops, pfrom);
		
		for(const auto&[pto, len]: umap) {
			size_t to = GetStopIndex(stops, pto);
			uint32_t distance = (to & 0x7FF) | (static_cast<uint32_t>(len) << 11);
			base.mutable_stop(from)->add_road_distance(distance);
		}
	}
	
	for(const transport::Bus& bus: transport_catalogue.GetBuses()) {
		Bus b;
		b.set_name(bus.name_);
		b.set_is_roundtrip(bus.circular_);
		//будем использовать по 11 бит на остановку (2048 различных остановок > 2000 запросов из условия задачи)
		//0x7FF будем использовать как запрещенный номер
		/*size_t size_temp = (bus.stops_.size() * 11 + 31) / 32;
		std::vector<uint32_t> temp(size_temp);
		size_t offset = 0;
		for(const transport::Stop* stop: bus.stops_) {
			auto stop_id = GetStopIndex(stops, stop);
			assert(stop_id < 0x7FF);
			size_t index = offset / 32;
			size_t offset_in_index = offset % 32;
			temp[index] |= stop_id << offset_in_index;
			if(offset_in_index > 21) {//был обрезан верх
				temp[index + 1] |= stop_id >> (32 - offset_in_index);
			}
			offset += 11;
		}
		
		//надо заполнить остатки битами
		temp.back() |= std::numeric_limits<uint32_t>::max() << (offset % 32);*/
		
		for(const transport::Stop* stop: bus.stops_) {
			auto stop_id = GetStopIndex(stops, stop);
			b.add_stop_id(stop_id);
		}
		*base.add_bus() = std::move(b);
	}
	
	return base;
}

void SaveTransportCatalogueTo(const transport::TransportCatalogue& transport_catalogue, std::ostream& output) {
	transport::serialize::TransportCatalogue save;
	
	*save.mutable_base() = SerializeBase(transport_catalogue);
	
	save.SerializeToOstream(&output);
}

}
}