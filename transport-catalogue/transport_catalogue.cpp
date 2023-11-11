#include "transport_catalogue.h"
#include <cassert>
#include <stdexcept>

using namespace std;

namespace tc::catalogue {

void TransportCatalogue::AddStop(const string& name, const geo::Coordinates& coordinates) {
    Stop stop;
    stop.name = name;
    stop.coordinates = coordinates;
    auto& new_stop = stops_.emplace_back(std::move(stop));
    stops_map_[new_stop.name] = &new_stop;
}

void TransportCatalogue::AddBus(const string& name, const vector<string_view> &stops) {
    Bus bus;
    bus.name = name;
    auto &new_bus = buses_.emplace_back(std::move(bus));
    buses_map_[new_bus.name] = &new_bus;

    new_bus.stops.reserve(stops.size());
    for (auto stop_name: stops) {
        assert(stops_map_.count(stop_name));
        auto *stop = stops_map_[stop_name];
        stop->buses.emplace(&new_bus);
        new_bus.stops.emplace_back(stop);
        new_bus.unique_stops.emplace(stop->name);
    }
}

void TransportCatalogue::SetDistance(std::string_view firstStopName, std::string_view secondStopName, int distance)
{
    auto* firstStop = stops_map_.count(firstStopName) ? stops_map_.at(firstStopName) : nullptr;
    assert(firstStop);
    auto* secondStop = stops_map_.count(secondStopName) ? stops_map_.at(secondStopName) : nullptr;
    assert(secondStop);
    stops_distances_.emplace(std::pair{firstStop, secondStop}, distance);
}

std::optional<TransportCatalogue::BusInfo> TransportCatalogue::GetBusInfo(string_view busName) const {
    if (buses_map_.count(busName) == 0) {
        return {};
    }
    auto* cur_bus = buses_map_.at(busName);
    assert(cur_bus);
    BusInfo bus_info;
    bus_info.name = cur_bus->name;
    bus_info.stops_count = cur_bus->stops.size();
    bus_info.unique_stops_count = cur_bus->unique_stops.size();

    double route_len_geo = 0;

    auto stops_begin = cur_bus->stops.begin();
    auto stops_end = cur_bus->stops.end();
    for (auto stops_it = stops_begin; stops_it != stops_end; ++stops_it) {
        if(stops_it == stops_begin) {
            continue;
        }
        auto* first_stop = *std::prev(stops_it);
        assert(first_stop);
        auto* second_stop = *stops_it;
        assert(second_stop);
        auto dist_geo = ComputeDistance(first_stop->coordinates, second_stop->coordinates);
        route_len_geo += dist_geo;

        if(auto key = pair{first_stop, second_stop}; stops_distances_.count(key)) {
            bus_info.route_length += stops_distances_.at(key);
        } else if(key = pair{second_stop, first_stop}; stops_distances_.count(key)) {
            bus_info.route_length += stops_distances_.at(key);
        } else {
            bus_info.route_length += dist_geo;
        }
    }

    assert(route_len_geo != 0);
    bus_info.curvature = bus_info.route_length / route_len_geo;

    return bus_info;
}

std::optional<TransportCatalogue::StopInfo> TransportCatalogue::GetStopInfo(string_view stopName) const {
    if (stops_map_.count(stopName) == 0) {
        return {};
    }
    auto* cur_stop = stops_map_.at(stopName);
    assert(cur_stop);

    StopInfo stop_info;
    stop_info.name = cur_stop->name;
    for(const auto* bus: cur_stop->buses) {
        stop_info.buses.emplace(bus->name);
    }

    return stop_info;
}

}