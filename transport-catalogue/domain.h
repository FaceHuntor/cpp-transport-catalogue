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

}