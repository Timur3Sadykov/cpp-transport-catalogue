#include "request_handler.h"

using namespace transport;

//RequestHandler::RequestHandler(Catalogue& catalogue)
//    : catalogue_(catalogue) {}

RequestHandler::RequestHandler(Catalogue& catalogue, MapRenderer& renderer)
    : catalogue_(catalogue), renderer_(renderer)
{}

void RequestHandler::AddStopRequest(std::string name, Coordinates coordinates, std::map<std::string_view, int> road_distances)
{
    stop_update_requests_.push_back( {std::move(name), std::move(coordinates), std::move(road_distances)});
}

void RequestHandler::AddBusRequest(std::string name, std::vector<std::string_view> stops, bool is_roundtrip)
{
    bus_update_requests_.push_back({std::move(name), is_roundtrip, std::move(stops)});
}

void RequestHandler::UpdateCatalogue()
{
    for (const auto& request : stop_update_requests_)
    {
        catalogue_.AddStop(request.name, request.coordinates);
    }

    for (const auto& request : stop_update_requests_)
    {
        for (const auto& [stop_name_to, distance] : request.road_distances)
        {
            catalogue_.SetStopsDistance(request.name, stop_name_to, distance);
        }
    }

    for (const auto& request : bus_update_requests_)
    {
        catalogue_.AddBus(request.name, request.stops, request.is_roundtrip);
    }
}

std::optional<StopInfo> RequestHandler::GetStopInfo(std::string_view name_view) const
{
    return catalogue_.FindStop(name_view);
}

std::optional<BusInfo> RequestHandler::GetBusInfo(std::string_view name_view) const
{
    return catalogue_.FindBus(name_view);
}

void RequestHandler::SetRendererSettings(RenderSettings render_settings)
{
    renderer_.SetRenderSettings(std::move(render_settings));
}

svg::Document RequestHandler::RenderMap() const
{
    return renderer_.Render(catalogue_.GetBuses());
}