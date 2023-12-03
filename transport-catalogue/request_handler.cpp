#include "request_handler.h"

namespace tc {

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer) :
    db_(db), renderer_(renderer) { }

std::optional<catalogue::BusInfo> RequestHandler::GetBusInfo(std::string_view bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<catalogue::StopInfo> RequestHandler::GetStopInfo(std::string_view stop_name) const {
    return db_.GetStopInfo(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap(db_.GetBuses().begin(), db_.GetBuses().end(),
                               db_.GetStops().begin(), db_.GetStops().end());
}

}

