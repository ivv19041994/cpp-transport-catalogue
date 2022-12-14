#pragma once
#include <string>
#include <list>
#include <unordered_set>

#include "geo.h"
namespace transport {
	struct Stop {
		std::string name_;
		geo::Coordinates coordinates_;
	};

	using ConteinerOfStopPointers = std::list<Stop*>;

	struct Bus {
		std::string name_;
		bool circular_;
		ConteinerOfStopPointers stops_;
		std::unordered_set<Stop*> stops_set_;
	};

	double ComputeDistance(const Stop*, const Stop*);


}//namespace transport