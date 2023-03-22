#include <iostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string_view>
#include "json_reader.h"
#include "serialization.h"
#include "json.h"
#include <filesystem>

using namespace std;
using namespace transport;




using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);
	
	//загружая json сдесь мы делаем InputStatReader не зависимым от сериализации
	::json::Document document = ::json::Load(cin);
	std::filesystem::path db_path = document.GetRoot()
	.AsDict().at("serialization_settings")
	.AsDict().at("file")
	.AsString();

    if (mode == "make_base"sv) {
		TransportCatalogue tc;
		
		transport::json::InputStatReader{}(document, cout, tc);
		fstream file(db_path, ios::binary | ios::out);
        transport::serialize::SaveTransportCatalogueTo(tc, file);
		
    } else if (mode == "process_requests"sv) {
		fstream file(db_path, ios::binary | ios::in);
		TransportCatalogue tc = transport::serialize::DeserializeTransportCatalogue(file);
		transport::json::InputStatReader{}(document, cout, tc);
        // process requests here

    } else {
        PrintUsage();
        return 1;
    }
}