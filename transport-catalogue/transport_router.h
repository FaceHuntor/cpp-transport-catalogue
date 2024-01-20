#pragma once
#include <string_view>
#include <vector>
#include <variant>
#include <optional>

#include "router.h"
#include "domain.h"

namespace tc::routing {

class TransportRouter {
public:
    struct RouterSettings {
        double bus_velocity;
        int bus_wait_time;
    };

    struct Route {
        struct WaitItem {
            std::string_view stop_name;
            double time;
        };

        struct BusItem {
            std::string_view bus;
            int span_count;
            double time;
        };

        double total_time;
        std::vector<std::variant<WaitItem, BusItem>> items;
    };

    explicit TransportRouter(RouterSettings settings = {});

    TransportRouter& SetSettings(RouterSettings settings);

    template <typename BusInputIt, typename StopInputIt, typename DistanceGetter>
    TransportRouter& SetData(BusInputIt buses_begin, BusInputIt buses_end,
                             StopInputIt stops_begin, StopInputIt stops_end,
                             const DistanceGetter& distance_getter);

    [[nodiscard]] std::optional<Route> GetRoute(std::string_view from, std::string_view to) const;

private:
    void Reset();

private:
    using EdgeWeight = double;

    struct WeightInfo {
        double weight = 0;
        std::string_view bus_name;
        int count = 0;
    };

    RouterSettings settings_;
    std::vector<std::string_view> stop_names_;
    std::unordered_map<std::string_view, size_t> nodes_map_;
    std::unordered_map<std::pair<size_t, size_t>, WeightInfo,
            domain::OrderedPairHasher<size_t, size_t>> weight_map_;

    std::optional<graph::DirectedWeightedGraph<EdgeWeight>> graph_;
    std::optional<graph::Router<EdgeWeight>> router_;
};

template<typename BusInputIt, typename StopInputIt, typename DistanceGetter>
TransportRouter& TransportRouter::SetData(BusInputIt buses_begin, BusInputIt buses_end,
                                          StopInputIt stops_begin, StopInputIt stops_end,
                                          const DistanceGetter& distance_getter) {
    Reset();

    if(buses_begin == buses_end || stops_begin == stops_end) {
        return *this;
    }

    for(auto bus_it = buses_begin; bus_it != buses_end; ++bus_it) {
        if(bus_it->stops.size() < 2) {
            continue;
        }
        std::vector<double> weights_from_first_stop;
        weights_from_first_stop.reserve(bus_it->stops.size());
        weights_from_first_stop.emplace_back(0);

        for(const auto* stop: bus_it->stops){
            if(!nodes_map_.count(stop->name)) {
                auto id = stop_names_.size();
                stop_names_.emplace_back(stop->name);
                nodes_map_[stop->name] = id;
            }
        }

        double sum_weight = 0;
        for(size_t i = 1; i < bus_it->stops.size(); ++i) {
            const auto* cur_stop = bus_it->stops[i];
            const auto* prev_stop = bus_it->stops[i - 1];
            auto distance = distance_getter(prev_stop->name, cur_stop->name);

            sum_weight += (*distance) / settings_.bus_velocity;
            weights_from_first_stop.emplace_back(sum_weight);
        }

        for(size_t i = 0; i < bus_it->stops.size() - 1; ++i) {
            const auto* stop_from = bus_it->stops[i];
            auto id_from = nodes_map_.at(stop_from->name);

            for(size_t j = i + 1; j < bus_it->stops.size(); ++j) {
                const auto* stop_to = bus_it->stops[j];
                auto id_to = nodes_map_.at(stop_to->name);

                double weight = weights_from_first_stop[j] - weights_from_first_stop[i];
                size_t stop_count = j - i;

                auto& weight_info =  weight_map_[{id_from, id_to}];
                if(weight_info.count == 0 || weight < weight_info.weight)
                {
                    weight_info.weight = weight;
                    weight_info.bus_name = bus_it->name;
                    weight_info.count = stop_count;
                }
            }
        }
    }

    graph_.emplace(stop_names_.size() * 2);

    auto wait_time = double(settings_.bus_wait_time);
    for(size_t id = 0; id < stop_names_.size(); ++id) {
        graph_->AddEdge({id * 2 + 1, id * 2, wait_time});
    }
    for(const auto& [ids, weight_info]: weight_map_) {
        graph_->AddEdge({ids.first * 2, ids.second * 2 + 1, weight_info.weight});
    }
    router_.emplace(*graph_);

    return *this;
}

}