#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace tc {

class RequestHandler {
public:
    RequestHandler(const catalogue::TransportCatalogue& db,
                   const renderer::MapRenderer& renderer,
                   const routing::TransportRouter& router);

    std::optional<catalogue::BusInfo> GetBusInfo(std::string_view bus_name) const;

    std::optional<catalogue::StopInfo> GetStopInfo(std::string_view stop_name) const;

    std::optional<routing::TransportRouter::Route> GetRoute(std::string_view from, std::string_view to) const;

    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const routing::TransportRouter& router_;
};

}