#pragma once
#include "geo.h"
#include <string>
#include <vector>
#include <set>
#include <string_view>

namespace transport {
    struct Stop {
        std::string name;
        Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        bool is_roundtrip;
        std::vector<const Stop*> bus_stops;
    };

    struct BusInfo {
        std::string_view name;
        int stops_on_route, unique_stops;
        int route_length;
        double curvature;

    };

    struct StopInfo {
        std::string_view name;
        const std::set<std::string_view>& buses_names;
    };
}