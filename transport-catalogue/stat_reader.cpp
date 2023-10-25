#include <regex>
#include <iomanip>
#include "stat_reader.h"

using namespace std;

namespace tc::io {

using namespace catalogue;

void PrintBusStat(const TransportCatalogue &transport_catalogue, ostream &output, string_view id) {
    const auto *bus = transport_catalogue.GetBus(id);
    output << "Bus " << id << ": ";
    if (!bus) {
        output << "not found";
    } else {
        output << bus->stops.size() << " stops on route, " << bus->unique_stops.size() << " unique stops, "
               << setprecision(6) << bus->route_len << " route length";
    }
    output << endl;
}

void PrintStopStat(const TransportCatalogue &transport_catalogue, ostream &output, string_view id) {
    const auto *stop = transport_catalogue.GetStop(id);
    output << "Stop " << id << ": ";
    if (!stop) {
        output << "not found";
    } else if (stop->buses.empty()) {
        output << "no buses";
    } else {
        output << "buses";
        for (const auto &bus: stop->buses) {
            output << " " << bus->name;
        }
    }
    output << endl;
}

void ParseAndPrintStat(const TransportCatalogue &transport_catalogue, string_view request,
                       ostream &output) {
    auto space_pos = request.find(' ');
    if (space_pos == string::npos) {
        return;
    }

    string_view command(request.begin(), space_pos);

    auto id_with_spaces = request.substr(space_pos + 1);
    auto id_start = id_with_spaces.find_first_not_of(' ');
    if (id_start == string::npos) {
        return;
    }
    auto id_with_post_spaces = id_with_spaces.substr(id_start);
    auto id_end = id_with_post_spaces.find_last_not_of(' ');

    auto id = id_with_post_spaces.substr(0, id_end + 1);
    if (command == "Bus") {
        PrintBusStat(transport_catalogue, output, id);
    } else if (command == "Stop") {
        PrintStopStat(transport_catalogue, output, id);
    }
}

}