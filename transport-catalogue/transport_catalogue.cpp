#include "transport_catalogue.h"
#include <cassert>
#include <stdexcept>
#include <iostream>

using namespace std;

namespace tc::catalogue {

void TransportCatalogue::AddStop(const string& name, const geo::Coordinates& coordinates) {
    domain::Stop stop;
    stop.name = name;
    stop.coordinates = coordinates;
    auto& new_stop = stops_.emplace_back(std::move(stop));
    stops_map_[new_stop.name] = &new_stop;
}

void TransportCatalogue::AddBus(const string& name, const vector<string_view> &stops, bool is_roundtrip) {
    domain::Bus bus;
    bus.name = name;
    bus.is_roundtrip = is_roundtrip;
    auto &new_bus = buses_.emplace_back(std::move(bus));
    buses_map_[new_bus.name] = &new_bus;

    new_bus.stops.reserve(stops.size());
    for (auto stop_name: stops) {
        assert(stops_map_.count(stop_name));
        auto *stop = stops_map_[stop_name];
        stop->buses.emplace(&new_bus);
        new_bus.stops.emplace_back(stop);
    }
}

void TransportCatalogue::SetDistance(std::string_view first_stop_name, std::string_view second_stop_name, double distance)
{
    auto* first_stop = stops_map_.count(first_stop_name) ? stops_map_.at(first_stop_name) : nullptr;
    assert(first_stop);
    auto* second_stop = stops_map_.count(second_stop_name) ? stops_map_.at(second_stop_name) : nullptr;
    assert(second_stop);
    stops_distances_.emplace(std::pair{first_stop, second_stop}, distance);
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    if (buses_map_.count(bus_name) == 0) {
        return {};
    }
    auto* cur_bus = buses_map_.at(bus_name);
    assert(cur_bus);
    BusInfo bus_info;
    bus_info.name = cur_bus->name;
    bus_info.stops_count = cur_bus->stops.size();
    bus_info.unique_stops_count = std::unordered_set<const domain::Stop *>(cur_bus->stops.begin(), cur_bus->stops.end()).size();

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
        auto dist_geo = tc::geo::ComputeDistance(first_stop->coordinates, second_stop->coordinates);
        route_len_geo += dist_geo;

        if(stops_distances_.count(pair{first_stop, second_stop})) {
            bus_info.route_length += stops_distances_.at(pair{first_stop, second_stop});
        } else if(stops_distances_.count(pair{second_stop, first_stop})) {
            bus_info.route_length += stops_distances_.at(pair{second_stop, first_stop});
        } else {
            bus_info.route_length += dist_geo;
        }
    }

    assert(route_len_geo != 0);
    bus_info.curvature = bus_info.route_length / route_len_geo;
    return bus_info;
}

std::optional<StopInfo> TransportCatalogue::GetStopInfo(string_view stop_name) const {
    if (stops_map_.count(stop_name) == 0) {
        return {};
    }
    auto* cur_stop = stops_map_.at(stop_name);
    assert(cur_stop);

    StopInfo stop_info;
    stop_info.name = cur_stop->name;
    for(const auto* bus: cur_stop->buses) {
        stop_info.buses.emplace(bus->name);
    }

    return stop_info;
}

const TransportCatalogue::Stops& TransportCatalogue::GetStops() const {
    return stops_;
}

const TransportCatalogue::Buses& TransportCatalogue::GetBuses() const {
    return buses_;
}

std::optional<double> TransportCatalogue::GetDistance(std::string_view first_stop_name, std::string_view second_stop_name) const {
    if(!stops_map_.count(first_stop_name) || !stops_map_.count(second_stop_name)){
        return std::nullopt;
    }
    const auto* first_stop = stops_map_.at(first_stop_name);
    const auto* second_stop = stops_map_.at(second_stop_name);
    if(!stops_distances_.count({first_stop, second_stop})) {
        if(!stops_distances_.count({second_stop, first_stop})) {
            return std::nullopt;
        }
        return stops_distances_.at({second_stop, first_stop});
    }
    return stops_distances_.at({first_stop, second_stop});
}

}