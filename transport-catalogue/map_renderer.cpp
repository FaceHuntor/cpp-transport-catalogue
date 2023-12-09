#include "map_renderer.h"

#include <optional>


namespace tc::renderer {

MapRenderer::MapRenderer(tc::renderer::RenderSettings settings) : settings_(std::move(settings)) {

}

[[maybe_unused]] const RenderSettings& MapRenderer::GetSettings() const {
    return settings_;
}

void MapRenderer::SetSettings(tc::renderer::RenderSettings settings) {
    settings_ = std::move(settings);
}

void MapRenderer::RenderBusRoute(svg::Document& document, const SphereProjector& projector, const domain::Bus& bus, const svg::Color& color) const
{
    auto line = std::make_unique<svg::Polyline>();

    for(const auto stop: bus.stops){
        auto coords = projector(stop->coordinates);
        line->AddPoint(coords);
    }

    line->SetStrokeColor(color);
    line->SetFillColor(svg::Color());
    line->SetStrokeWidth(settings_.line_width);
    line->SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    line->SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    document.AddPtr(std::move(line));
}

void MapRenderer::RenderBusText(svg::Document& document, const std::string& text, const svg::Point& pos, const svg::Color& color) const
{
    auto base_text = std::make_unique<svg::Text>();

    base_text->SetData(text);
    base_text->SetPosition(pos);
    base_text->SetOffset(settings_.bus_label_offset);
    base_text->SetFontSize(settings_.bus_label_font_size);
    base_text->SetFontFamily("Verdana");
    base_text->SetFontWeight("bold");

    auto substrate = std::make_unique<svg::Text>(*base_text);

    substrate->SetFillColor(settings_.underlayer_color);
    substrate->SetStrokeColor(settings_.underlayer_color);
    substrate->SetStrokeWidth(settings_.underlayer_width);
    substrate->SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    substrate->SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    base_text->SetFillColor(color);
    document.AddPtr(std::move(substrate));
    document.AddPtr(std::move(base_text));
}

void MapRenderer::RenderStopSymbol(svg::Document& document, const svg::Point& pos) const
{
    auto stop_circle = std::make_unique<svg::Circle>();
    stop_circle->SetRadius(settings_.stop_radius);
    stop_circle->SetCenter(pos);
    stop_circle->SetFillColor(std::string("white"));
    document.AddPtr(std::move(stop_circle));
}

void MapRenderer::RenderStopText(svg::Document& document, const std::string& text, const svg::Point& pos) const
{
    auto stop_text = std::make_unique<svg::Text>();
    stop_text->SetPosition(pos);
    stop_text->SetOffset(settings_.stop_label_offset);
    stop_text->SetFontSize(settings_.stop_label_font_size);
    stop_text->SetFontFamily("Verdana");
    stop_text->SetData(text);

    auto stop_substrate = std::make_unique<svg::Text>(*stop_text);
    stop_substrate->SetFillColor(settings_.underlayer_color);
    stop_substrate->SetStrokeColor(settings_.underlayer_color);
    stop_substrate->SetStrokeWidth(settings_.underlayer_width);
    stop_substrate->SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    stop_substrate->SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    stop_text->SetFillColor(svg::Color(std::string("black")));

    document.AddPtr(std::move(stop_substrate));
    document.AddPtr(std::move(stop_text));
}

}