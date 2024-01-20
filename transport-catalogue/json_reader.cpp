#include "json_reader.h"
#include "json_builder.h"

#include <sstream>
#include <unordered_map>

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

class OutputFormer
{
public:
    virtual void Form(json::Builder& response, const json::Dict& request, const RequestHandler& handler) const = 0;

    virtual ~OutputFormer() = default;
};


class BusOutputFormer : public OutputFormer
{
public:
    void Form(json::Builder& response, const json::Dict& request, const RequestHandler& handler) const override
    {
        const auto& name = request.at("name").AsString();
        const auto bus_info = handler.GetBusInfo(name);
        if (!bus_info) {
            response.Key("error_message").Value("not found"s);
        } else {
            response.Key("curvature").Value(bus_info->curvature);
            response.Key("route_length").Value(bus_info->route_length);
            response.Key("stop_count").Value(static_cast<int>(bus_info->stops_count));
            response.Key("unique_stop_count").Value(static_cast<int>(bus_info->unique_stops_count));
        }
    }

    ~BusOutputFormer() override = default;
};

class StopOutputFormer : public OutputFormer
{
public:
    void Form(json::Builder& response, const json::Dict& request, const RequestHandler& handler) const override
    {
        const auto& name = request.at("name").AsString();
        const auto stop_info = handler.GetStopInfo(name);
        if (!stop_info) {
            response.Key("error_message").Value("not found"s);
        }
        else {
            response.Key("buses").StartArray();
            for (const auto &bus: stop_info->buses) {
                response.Value(std::string(bus));
            }
            response.EndArray();
        }
    }

    ~StopOutputFormer() override = default;
};

class RouteOutputFormer : public OutputFormer
{
private:
    struct RouteItemVisitor {
        json::Builder& response;

        void operator()(const routing::TransportRouter::Route::BusItem& item) {
            response.StartDict()
                    .Key("type").Value("Bus"s)
                    .Key("bus").Value(std::string(item.bus))
                    .Key("time").Value(item.time)
                    .Key("span_count").Value(item.span_count)
                    .EndDict();
        }
        void operator()(const routing::TransportRouter::Route::WaitItem& item) {
            response.StartDict()
                    .Key("type").Value("Wait"s)
                    .Key("time").Value(item.time)
                    .Key("stop_name").Value(std::string(item.stop_name.data()))
                    .EndDict();
        }
    };

public:
    void Form(json::Builder& response, const json::Dict& request, const RequestHandler& handler) const override
    {
        const auto& from = request.at("from").AsString();
        const auto& to = request.at("to").AsString();
        auto route = handler.GetRoute(from, to);
        if(!route) {
            response.Key("error_message").Value("not found"s);
        } else {
            response.Key("total_time").Value(route->total_time);
            auto items = response.Key("items").StartArray();
            for(const auto& item: route->items) {
                std::visit(RouteItemVisitor{response}, item);
            }
            items.EndArray();
        }
    }

    ~RouteOutputFormer() override = default;


};

class MapOutputFormer : public OutputFormer
{
public:
    void Form(json::Builder& response, [[maybe_unused]] const json::Dict& request, const RequestHandler& handler) const override
    {
        std::ostringstream ss;
        handler.RenderMap().Render(ss);
        response.Key("map").Value(ss.str());
    }

    ~MapOutputFormer() override = default;
};


void JsonReader::ParseInput(std::istream& input) {
    requests_ = json::Load(input);
}

void JsonReader::ApplyCommands(catalogue::TransportCatalogue& db,
                               renderer::MapRenderer& renderer,
                               routing::TransportRouter& router) const {
    auto& requests_map = requests_.GetRoot().AsMap();
    FillCatalogue(db, requests_map.at("base_requests").AsArray());
    SettingRenderer(renderer, requests_map.at("render_settings").AsMap());
    SettingRouter(router, requests_map.at("routing_settings").AsMap());
    router.SetData(db.GetBuses().begin(), db.GetBuses().end(),
                   db.GetStops().begin(), db.GetStops().end(),
                   [&db](std::string_view stop1, std::string_view stop2) {
                       return db.GetDistance(stop1, stop2);
    });
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

void JsonReader::SettingRouter(routing::TransportRouter& router, const json::Dict& requests) {
    routing::TransportRouter::RouterSettings settings{};

    if (requests.count("bus_wait_time")) {
        settings.bus_wait_time = requests.at("bus_wait_time").AsInt();
    }
    if (requests.count("bus_velocity")) {
        settings.bus_velocity = requests.at("bus_velocity").AsDouble() * 1000.0 / 60.0;
    }

    router.SetSettings(std::move(settings));
}

void JsonReader::FormOutput(const RequestHandler& handler, const json::Array& requests, std::ostream& output) {
    static const BusOutputFormer busOutputFormer;
    static const StopOutputFormer stopOutputFormer;
    static const RouteOutputFormer routeOutputFormer;
    static const MapOutputFormer mapOutputFormer;
    static const std::unordered_map<std::string_view, const OutputFormer&> outputFormers = {
            {"Bus"sv, busOutputFormer},
            {"Stop"sv, stopOutputFormer},
            {"Route"sv, routeOutputFormer},
            {"Map"sv, mapOutputFormer}
    };

    json::Builder out_builder;
    out_builder.StartArray();

    for(const auto& request_node: requests) {
        auto& request = request_node.AsMap();
        const auto& type = request.at("type").AsString();
        json::Builder response_unit;
        response_unit.StartDict();
        response_unit.Key("request_id").Value(request.at("id").AsInt());

        if(!outputFormers.count(type)) {
            continue;
        }
        outputFormers.at(type).Form(response_unit, request, handler);

        response_unit.EndDict();
        out_builder.Value(std::move(response_unit.Build()));
    }
    out_builder.EndArray();
    json::Print(json::Document(out_builder.Build()), output);
}

}