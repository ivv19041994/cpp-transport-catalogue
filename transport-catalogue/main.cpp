#include <iostream>
#include <sstream>

//#include "input_reader.h"
//#include "stat_reader.h"
#include "json_reader.h"

using namespace std;
using namespace transport;

int main()
{

    TransportCatalogue tc;
    transport::json::InputStatReader(cin, cout, tc);
    //transport::iostream::InputReader(cin, tc);
    //transport::iostream::StatReader(cin, cout, tc);
    //transport::ws::InputReader(web_socket, tc);
    //transport::ws::StatReader(web_socket, tc);
    //transport::http::InputReader(http_socket, tc);
    //transport::http::StatReader(http_socket, tc);
    //transport::iostream::ini::InputReader(ifile, tc);
    //transport::iostream::ini::StatReader(ifile, ofile, tc);
}
