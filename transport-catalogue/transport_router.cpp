#include <utility>
#include "transport_router.h"

namespace tc::routing {

TransportRouter::TransportRouter(TransportRouter::RouterSettings settings) : settings_(std::move(settings)) {

}

TransportRouter& TransportRouter::SetSettings(RouterSettings settings) {
    settings_ = std::move(settings);
    return *this;
}

std::optional<TransportRouter::Route> TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
    std::optional<TransportRouter::Route> result_route;
    if(!router_) {
        return result_route;
    }

    if(!nodes_map_.count(from) || !nodes_map_.count(to)) {
        return result_route;
    }

    auto route_info = router_->BuildRoute(nodes_map_.at(from) * 2 + 1,
                                          nodes_map_.at(to) * 2 + 1);
    if(!route_info) {
        return result_route;
    }

    result_route.emplace();
    result_route->total_time = route_info->weight;

    for(auto edge_id: route_info->edges) {
        const auto& edge = graph_->GetEdge(edge_id);
        // the first node is odd -> wait for bus
        if(edge.from & 1) {
            result_route->items.emplace_back(Route::WaitItem{stop_names_[edge.from >> 1], edge.weight});
        }
        // the first node is even -> move from stop to other stop
        else {
            const auto& weight_info = weight_map_.at({edge.from >> 1, edge.to >> 1});
            result_route->items.emplace_back(Route::BusItem{weight_info.bus_name, weight_info.count, weight_info.weight});
        }
    }

    return result_route;
}

void TransportRouter::Reset() {
    stop_names_.clear();
    nodes_map_.clear();
    router_.reset();
    graph_.reset();
}

}