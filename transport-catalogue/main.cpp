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

int make_base(const ::json::Document& document, const std::filesystem::path& db_path) {
	transport::json::BaseReader reader{};

	auto base = reader(document);
	fstream file(db_path, ios::binary | ios::out);
	transport::serialize::SaveTransportCatalogueTo(base.transport_catalogue, base.render_settings, base.router, file);
	return 0;
}

int process_requests(const ::json::Document& document, const std::filesystem::path& db_path) {
	fstream file(db_path, ios::binary | ios::in);
	transport::serialize::TransportCatalogue load;

	if (!load.ParseFromIstream(&file)) {
		throw std::logic_error("Data base is broken");
	}

	transport::json::Base base{
		transport::serialize::DeserializeBase(load.base()),
		transport::serialize::DeserializeRenderSettings(load.render_settings()),
		transport::serialize::DeserializeRouter(load.router())
	};

	transport::json::StatReader reader{ base };

	reader(document).GetRoot().Print(cout);
	return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);
	
	//загружая json сдесь мы делаем BaseReader/StatReader не зависимым от сериализации
	::json::Document document = ::json::Load(cin);
	std::filesystem::path db_path = document.GetRoot()
	.AsDict().at("serialization_settings")
	.AsDict().at("file")
	.AsString();

    if (mode == "make_base"sv) {
		return make_base(document, db_path);
    }
	if (mode == "process_requests"sv) {
		return process_requests(document, db_path);
    }

    PrintUsage();
    return 1;
}