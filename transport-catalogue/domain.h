#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace tc::domain {

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
    bool is_roundtrip;
};

}