#pragma once

#include <istream>

#include "transport_catalogue.h"

namespace transport {
	namespace iostream {
		void StatReader(std::istream& is, std::ostream& os, TransportCatalogue& transport_catalogue);
	}
}