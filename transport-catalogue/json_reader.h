#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "json.h"
#include "geo.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "request_handler.h"

namespace tc::io {

class JsonReader {
public:
    void ParseInput(std::istream& input);
    void ApplyCommands(catalogue::TransportCatalogue& db,
                       renderer::MapRenderer& renderer,
                       routing::TransportRouter& router) const;
    void GetOutput(const RequestHandler& handler, std::ostream& output) const;

private:
    static void FillCatalogue(catalogue::TransportCatalogue& db, const json::Array& requests);
    static void SettingRenderer(renderer::MapRenderer& rend, const json::Dict& requests);
    static void SettingRouter(routing::TransportRouter& router, const json::Dict& requests);
    static void FormOutput(const RequestHandler& handler, const json::Array& requests, std::ostream& output);

private:
    json::Document requests_ = json::Document{{}};
};

}