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

#include "geo.h"

namespace tc::catalogue {

class TransportCatalogue {
public:
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

public:
    TransportCatalogue() = default;

    ~TransportCatalogue() = default;

    void AddStop(const std::string& name, const geo::Coordinates& coordinates);

    void AddBus(const std::string& name, const std::vector<std::string_view> &stops);

    void SetDistance(std::string_view firstStop, std::string_view secondStop, int distance);

    std::optional<BusInfo> GetBusInfo(std::string_view busName) const;

    std::optional<StopInfo> GetStopInfo(std::string_view stopName) const;

private:
    template<typename T>
    void
    static hash_combine(std::size_t &seed, T const &key) {
        std::hash<T> hasher;
        seed ^= hasher(key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<typename T1, typename T2>
    struct OrderedPairHasher {
        std::size_t operator()(std::pair<T1, T2> const &p) const {
            std::size_t seed1(0);
            hash_combine(seed1, p.first);
            hash_combine(seed1, p.second);
            return seed1;
        }
    };

    struct Bus;

    struct Stop {
    public:
        std::string name;
        geo::Coordinates coordinates;
        std::unordered_set<const Bus *> buses;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop *> stops;
        std::unordered_set<std::string_view> unique_stops;
    };

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus *> buses_map_;
    std::unordered_map<std::string_view, Stop *> stops_map_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, double, OrderedPairHasher<const Stop*, const Stop*>> stops_distances_;
};

}