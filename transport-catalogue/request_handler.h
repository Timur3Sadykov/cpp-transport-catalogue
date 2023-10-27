#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "geo.h"
#include "svg.h"
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <optional>

namespace transport
{
    class RequestHandler
    {
    public:
        //explicit RequestHandler(Catalogue& catalogue);

        RequestHandler(Catalogue& catalogue, MapRenderer& renderer);

        void AddStopRequest(std::string name, Coordinates coordinates, std::map<std::string_view, int> road_distances);

        void AddBusRequest(std::string name, std::vector<std::string_view> stops, bool is_roundtrip);

        void UpdateCatalogue();

        std::optional<StopInfo> GetStopInfo(std::string_view name_view) const;

        std::optional<BusInfo> GetBusInfo(std::string_view name_view) const;

        void SetRendererSettings(RenderSettings render_settings);

        svg::Document RenderMap() const;

    private:
        struct StopUpdateRequest
        {
            std::string name;
            Coordinates coordinates;
            std::map<std::string_view, int> road_distances;
        };

        struct BusUpdateRequest
        {
            std::string name;
            bool is_roundtrip = false;
            std::vector<std::string_view> stops;
        };

        Catalogue& catalogue_;
        MapRenderer& renderer_;
        std::deque<StopUpdateRequest> stop_update_requests_;
        std::deque<BusUpdateRequest> bus_update_requests_;
    };
}