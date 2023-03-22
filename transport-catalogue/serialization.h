#pragma once
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"

namespace transport {
namespace serialize {
	
transport::TransportCatalogue DeserializeTransportCatalogue(std::istream& input);

void SaveTransportCatalogueTo(const transport::TransportCatalogue& transport_catalogue, std::ostream& output);

}
}