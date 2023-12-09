#pragma once

#include <vector>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <cassert>

#include "svg.h"
#include "domain.h"

namespace tc::renderer{

struct RenderSettings {
    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;

    int bus_label_font_size = 0;
    svg::Point bus_label_offset;

    int stop_label_font_size = 0;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width = 0;

    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
            : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    static constexpr double EPSILON = 1e-6;

    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    explicit MapRenderer(RenderSettings settings = {});

    template <typename BusInputIt, typename StopInputIt>
    svg::Document RenderMap(BusInputIt buses_begin, BusInputIt buses_end, StopInputIt stops_begin, StopInputIt stops_end) const;
    void SetSettings(RenderSettings settings);
    [[maybe_unused]] const RenderSettings& GetSettings() const;

private:
    void RenderBusRoute(svg::Document& document, const SphereProjector& projector, const domain::Bus& bus, const svg::Color& color) const;
    void RenderBusText(svg::Document& document, const std::string& text, const svg::Point& pos, const svg::Color& color) const;
    void RenderStopSymbol(svg::Document& document, const svg::Point& pos) const;
    void RenderStopText(svg::Document& document, const std::string& text, const svg::Point& pos) const;

private:
    RenderSettings settings_;
};

template <typename BusInputIt, typename StopInputIt>
svg::Document MapRenderer::RenderMap(BusInputIt buses_begin, BusInputIt buses_end, StopInputIt stops_begin, StopInputIt stops_end) const {
    std::vector<geo::Coordinates> stop_coords;
    for(auto stop_it = stops_begin; stop_it != stops_end; ++stop_it) {
        if(stop_it->buses.empty()){
            continue;
        }
        stop_coords.emplace_back(stop_it->coordinates);
    }

    SphereProjector projector(stop_coords.begin(), stop_coords.end(), settings_.width, settings_.height, settings_.padding);

    svg::Document document;

    std::vector<const domain::Bus*> buses;
    buses.reserve(std::distance(buses_begin, buses_end));
    std::transform(buses_begin, buses_end, std::inserter(buses, buses.end()), [](const domain::Bus& bus) {return &bus;});
    std::sort(buses.begin(), buses.end(), [](const domain::Bus* lhs, const domain::Bus* rhs) {return lhs->name < rhs->name;});

    std::vector<const domain::Stop*> stops;
    for(auto stop_it = stops_begin; stop_it != stops_end; ++stop_it) {
        if(stop_it->buses.empty()){
            continue;
        }
        stops.emplace_back(&(*stop_it));
    }
    std::sort(stops.begin(), stops.end(), [](const domain::Stop* lhs, const domain::Stop* rhs) {return lhs->name < rhs->name;});

    auto cur_palette_color = settings_.color_palette.begin();
    for(const auto& bus: buses) {
        if(bus->stops.empty()) {
            continue;
        }
        assert(cur_palette_color.base());

        RenderBusRoute(document, projector, *bus, *cur_palette_color);

        ++cur_palette_color;
        cur_palette_color = cur_palette_color == settings_.color_palette.end() ? settings_.color_palette.begin() : cur_palette_color;
    }

    cur_palette_color = settings_.color_palette.begin();
    for(const auto& bus: buses) {
        if(bus->stops.empty()) {
            continue;
        }
        {
            auto& stop = bus->stops.front();
            auto pos = projector(stop->coordinates);
            RenderBusText(document, bus->name, pos, *cur_palette_color);
        }
        if(!bus->is_roundtrip) {
            auto& last_stop = bus->stops[bus->stops.size() / 2];
            if(last_stop->name != bus->stops.front()->name) {
                auto pos = projector(last_stop->coordinates);
                RenderBusText(document, bus->name, pos, *cur_palette_color);
            }
        }
        ++cur_palette_color;
        cur_palette_color = cur_palette_color == settings_.color_palette.end() ? settings_.color_palette.begin() : cur_palette_color;
    }

    for(const auto& stop: stops){
        if(stop->buses.empty()){
            continue;
        }
        auto coords = projector(stop->coordinates);
        RenderStopSymbol(document, coords);
    }

    for(const auto& stop: stops){
        if(stop->buses.empty()){
            continue;
        }
        auto coords = projector(stop->coordinates);
        RenderStopText(document, stop->name, coords);
    }
    return document;
}


}

