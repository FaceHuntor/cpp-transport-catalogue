#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>
#include <set>
#include <optional>

#include "domain.h"
#include "geo.h"

namespace tc::catalogue {

struct StopInfo {
    std::string_view name;
    std::set<std::string_view> buses;
};

struct BusInfo {
    std::string_view name;
    size_t stops_count = 0;
    size_t unique_stops_count = 0;
    double route_length = 0;
    double curvature = 0;
};

class TransportCatalogue {

public:
    using Stops = std::deque<domain::Stop>;
    using Buses = std::deque<domain::Bus>;

public:
    TransportCatalogue() = default;

    ~TransportCatalogue() = default;

    void AddStop(const std::string& name, const geo::Coordinates& coordinates);

    void AddBus(const std::string& name, const std::vector<std::string_view> &stops, bool is_roundtrip);

    void SetDistance(std::string_view first_stop_name, std::string_view second_stop_name, double distance);

    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;

    std::optional<StopInfo> GetStopInfo(std::string_view stop_name) const;

    const Stops& GetStops() const;

    const Buses& GetBuses() const;

    std::optional<double> GetDistance(std::string_view first_stop_name, std::string_view second_stop_name) const;

private:
    Stops stops_;
    Buses buses_;
    std::unordered_map<std::string_view, domain::Bus *> buses_map_;
    std::unordered_map<std::string_view, domain::Stop *> stops_map_;
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, double,
                       domain::OrderedPairHasher<const domain::Stop*, const domain::Stop*>> stops_distances_;
};

}