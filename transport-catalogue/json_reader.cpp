#include "json_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>

namespace tc::io {

using namespace catalogue;
using namespace geo;
using namespace std::literals;

svg::Color TransformColor(const json::Node& color_node) {
    if(color_node.IsString()) {
        return {color_node.AsString()};
    }
    if(color_node.IsArray()) {
        auto& color = color_node.AsArray();
        if(color.size() == 3) {
            return svg::Rgb(color[0].AsInt(),
                            color[1].AsInt(),
                            color[2].AsInt());
        } else if(color.size() == 4) {
            return svg::Rgba(color[0].AsInt(),
                             color[1].AsInt(),
                             color[2].AsInt(),
                             color[3].AsDouble());
        }
    }
    assert(false);
    return {};
}

void JsonReader::ParseInput(std::istream& input) {
    requests_ = json::Load(input);
}

void JsonReader::ApplyCommands(catalogue::TransportCatalogue& db, renderer::MapRenderer& renderer) const {
    auto& requests_map = requests_.GetRoot().AsMap();
    FillCatalogue(db, requests_map.at("base_requests").AsArray());
    SettingRenderer(renderer, requests_map.at("render_settings").AsMap());
}

void JsonReader::GetOutput(const tc::RequestHandler& handler, std::ostream& output) const {
    auto& requests_map = requests_.GetRoot().AsMap();
    FormOutput(handler, requests_map.at("stat_requests").AsArray(), output);
}

void JsonReader::FillCatalogue(catalogue::TransportCatalogue& db, const json::Array& requests) {

    for (const auto &request_node: requests) {
        auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            const auto& name = request.at("name").AsString();
            db.AddStop(name,
                              {request.at("latitude").AsDouble(),
                               request.at("longitude").AsDouble()});
        }
    }

    for (const auto &request_node: requests) {
        auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            const auto& first_stop_name = request.at("name").AsString();
            for(const auto& [second_stop_name, dist]: request.at("road_distances").AsMap()) {
                db.SetDistance(first_stop_name, second_stop_name, dist.AsDouble());
            }
        }
    }

    for (const auto &request_node: requests) {
        auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Bus") {
            const auto& name = request.at("name").AsString();
            const auto& stops = request.at("stops").AsArray();
            assert(!stops.empty());
            std::vector<std::string_view> stops_view;
            bool is_roundtrip = request.count("is_roundtrip") && request.at("is_roundtrip").AsBool();
            stops_view.reserve(is_roundtrip ? stops.size() : stops.size() * 2 - 1);
            std::transform(stops.begin(), stops.end(), std::back_inserter(stops_view), [](const json::Node& node){return std::string_view(node.AsString());});
            if(!is_roundtrip) {
                std::transform(std::next(stops.rbegin()), stops.rend(), std::back_inserter(stops_view), [](const json::Node& node){return std::string_view(node.AsString());});
            }
            db.AddBus(name, stops_view, is_roundtrip);
        }
    }
}

void JsonReader::SettingRenderer(renderer::MapRenderer& rend, const json::Dict& requests) {
    renderer::RenderSettings settings;
    if (requests.count("width")) {
        settings.width = requests.at("width").AsDouble();
    }
    if (requests.count("height")) {
        settings.height = requests.at("height").AsDouble();
    }
    if (requests.count("padding")) {
        settings.padding = requests.at("padding").AsDouble();
    }
    if (requests.count("stop_radius")) {
        settings.stop_radius = requests.at("stop_radius").AsDouble();
    }
    if (requests.count("line_width")) {
        settings.line_width = requests.at("line_width").AsDouble();
    }


    if (requests.count("bus_label_font_size")) {
        settings.bus_label_font_size = requests.at("bus_label_font_size").AsInt();
    }
    if (requests.count("bus_label_offset")) {
        const auto& param = requests.at("bus_label_offset").AsArray();
        assert(param.size() == 2);
        settings.bus_label_offset = {param[0].AsDouble(), param[1].AsDouble()};
    }

    if (requests.count("stop_label_font_size")) {
        settings.stop_label_font_size = requests.at("stop_label_font_size").AsInt();
    }
    if (requests.count("stop_label_offset")) {
        const auto& param = requests.at("stop_label_offset").AsArray();
        assert(param.size() == 2);
        settings.stop_label_offset = {param[0].AsDouble(), param[1].AsDouble()};
    }

    if (requests.count("underlayer_color")) {
        const auto& param = requests.at("underlayer_color");
        settings.underlayer_color = TransformColor(param);
    }

    if (requests.count("underlayer_width")) {
        settings.underlayer_width = requests.at("underlayer_width").AsDouble();
    }

    if (requests.count("color_palette")) {

        for(const auto& color_in_palette: requests.at("color_palette").AsArray()) {
            settings.color_palette.emplace_back(TransformColor(color_in_palette));
        }
    }
    rend.SetSettings(settings);
}

void JsonReader::FormOutput(const RequestHandler& handler, const json::Array& requests, std::ostream& output) {
    json::Array out_data;

    for(const auto& request_node: requests) {
        auto& request = request_node.AsMap();
        const auto& type = request.at("type").AsString();

        json::Dict response_unit;
        response_unit["request_id"] = request.at("id").AsInt();

        if (type == "Bus") {
            const auto& name = request.at("name").AsString();
            const auto bus_info = handler.GetBusInfo(name);
            if (!bus_info) {
                response_unit["error_message"] = "not found"s;
            } else {
                response_unit["curvature"] = bus_info->curvature;
                response_unit["route_length"] = bus_info->route_length;
                response_unit["stop_count"] = static_cast<int>(bus_info->stops_count);
                response_unit["unique_stop_count"] = static_cast<int>(bus_info->unique_stops_count);
            }
        } else if (type == "Stop") {
            const auto& name = request.at("name").AsString();
            const auto stop_info = handler.GetStopInfo(name);
            if (!stop_info) {
                response_unit["error_message"] = "not found"s;
            }
            else {
                json::Array buses;
                for (const auto &bus: stop_info->buses) {
                    buses.emplace_back(std::string(bus));
                }
                response_unit["buses"] = std::move(buses);
            }
        } else if (type == "Map") {
            std::ostringstream ss;
            handler.RenderMap().Render(ss);
            response_unit["map"] = ss.str();
        } else {
            continue;
        }
        out_data.emplace_back(std::move(response_unit));
    }
    json::Print(json::Document(std::move(out_data)), output);
}

}