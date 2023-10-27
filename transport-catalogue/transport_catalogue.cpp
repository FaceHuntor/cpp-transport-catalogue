#include "transport_catalogue.h"
#include <cassert>

using namespace std;

namespace tc::catalogue {

void TransportCatalogue::AddStop(const string& name, geo::Coordinates coordinates) {
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
    for (size_t i = 0; i < stops.size(); ++i) {
        const auto &stop_name = stops[i];
        assert(stops_map_.count(stop_name));
        auto *stop = stops_map_[stop_name];
        stop->buses.emplace(&new_bus);
        new_bus.stops.emplace_back(stop);
        new_bus.unique_stops.emplace(stop->name);
        if (i > 0) {
            new_bus.route_len += ComputeDistance(new_bus.stops[i - 1]->coordinates, new_bus.stops[i]->coordinates);
        }
    }
}

const TransportCatalogue::Bus *TransportCatalogue::GetBus(string_view id) const {
    if (buses_map_.count(id) == 0) {
        return nullptr;
    }
    return buses_map_.at(id);
}

const TransportCatalogue::Stop *TransportCatalogue::GetStop(string_view id) const {
    if (stops_map_.count(id) == 0) {
        return nullptr;
    }
    return stops_map_.at(id);
}

}