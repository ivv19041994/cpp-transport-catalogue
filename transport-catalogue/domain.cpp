#include "domain.h"

namespace transport {

	double ComputeDistance(const Stop* lhs, const Stop* rhs) {
		return geo::ComputeDistance(lhs->coordinates_, rhs->coordinates_);
	}


}//namespace transport