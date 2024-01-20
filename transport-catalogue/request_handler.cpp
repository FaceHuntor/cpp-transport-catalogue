#include "request_handler.h"

namespace tc {

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db,
                               const renderer::MapRenderer& renderer,
                               const routing::TransportRouter& router) :
    db_(db), renderer_(renderer), router_(router) { }

std::optional<catalogue::BusInfo> RequestHandler::GetBusInfo(std::string_view bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<catalogue::StopInfo> RequestHandler::GetStopInfo(std::string_view stop_name) const {
    return db_.GetStopInfo(stop_name);
}

std::optional<routing::TransportRouter::Route>
RequestHandler::GetRoute(std::string_view from, std::string_view to) const {
    return router_.GetRoute(from, to);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap(db_.GetBuses().begin(), db_.GetBuses().end(),
                               db_.GetStops().begin(), db_.GetStops().end());
}



}

