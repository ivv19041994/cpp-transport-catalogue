#pragma once

#include <istream>

#include "transport_catalogue.h"

namespace transport {
	namespace iostream {
		std::istream& InputReader(std::istream& is, transport::TransportCatalogue& transport_catalogue);
	}
}
