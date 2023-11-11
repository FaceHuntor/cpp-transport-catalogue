#include <regex>
#include <iomanip>
#include "stat_reader.h"

using namespace std;

namespace tc::io {

using namespace catalogue;

void PrintBusStat(const TransportCatalogue &transport_catalogue, ostream &output, string_view bus_name) {
    auto bus_info = transport_catalogue.GetBusInfo(bus_name);
    output << "Bus " << bus_name << ": ";
    if (!bus_info) {
        output << "not found";
    } else {
        output << bus_info->stops_count << " stops on route, " << bus_info->unique_stops_count << " unique stops, "
               << setprecision(0) << fixed << bus_info->route_length << " route length, "
               << setprecision(5) << fixed << bus_info->curvature << " curvature";
    }
    output << endl;
}

void PrintStopStat(const TransportCatalogue &transport_catalogue, ostream &output, string_view stop_name) {
    const auto stop_info = transport_catalogue.GetStopInfo(stop_name);
    output << "Stop " << stop_name << ": ";
    if (!stop_info) {
        output << "not found";
    } else if (stop_info->buses.empty()) {
        output << "no buses";
    } else {
        output << "buses";
        for (const auto &bus: stop_info->buses) {
            output << " " << bus;
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