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

}