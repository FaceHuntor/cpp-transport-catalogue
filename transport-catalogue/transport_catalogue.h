#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>

#include "geo.h"

namespace tc::catalogue {

class TransportCatalogue {
public:
    struct Stop;
    struct Bus;

    struct Stop {
    private:
        struct BusComparator {
            bool operator()(const Bus *lhs, const Bus *rhs) const {
                return lhs->name < rhs->name;
            }
        };

    public:
        std::string name;
        geo::Coordinates coordinates;
        std::set<const Bus *, BusComparator> buses;

        Stop() = default;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop *> stops;
        std::unordered_set<std::string_view> unique_stops;
        double route_len = 0;
    };

public:
    TransportCatalogue() = default;

    ~TransportCatalogue() = default;

    void AddStop(std::string id, geo::Coordinates coordinates);

    void AddBus(std::string id, const std::vector<std::string_view> &stops);

    const Bus *GetBus(std::string_view id) const;

    const Stop *GetStop(std::string_view id) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus *> buses_map_;
    std::unordered_map<std::string_view, Stop *> stops_map_;
};

}