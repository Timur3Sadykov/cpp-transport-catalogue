#pragma once

#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <set>
#include <unordered_map>
#include <set>

namespace transport {
    using namespace std::string_literals;

    struct BusInfo {
        std::string name;
        size_t stops_on_route, unique_stops;
        int route_length;
        double curvature;

    };

    struct StopInfo {
        std::string name;
        std::set<std::string_view> buses_names;
    };

    class Catalogue {
    public:
        void AddStop(std::string_view name_view, double latitude, double longitude);

        void AddStopsDistance(std::string_view stop_name_1, std::string_view stop_name_2, int distance);

        void
        AddBus(std::string_view name_view, const std::vector<std::string_view> &stops_names, bool is_circular = false);

        BusInfo FindBus(std::string_view name_view) const;

        StopInfo FindStop(std::string_view name_view) const;

    private:
        struct Stop {
            std::string name;
            double latitude, longitude;
        };

        struct Bus {
            std::string name;
            std::vector<const Stop *> bus_stops;
        };

        class StopPtrPairHasher {
        public:
            size_t operator()(const std::pair<const Stop *, const Stop *> &pair) const {
                size_t h_first = hasher_(pair.first);
                size_t h_second = hasher_(pair.second);

                return h_first + h_second * 37;
            }

        private:
            std::hash<const Stop *> hasher_;
        };

        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, const Stop *> stopname_to_stop_;
        std::deque<Bus> buses_;
        std::unordered_map<std::string_view, const Bus *> busname_to_bus_;
        std::unordered_map<const Stop *, std::set<std::string_view>> stop_to_buses_names;
        std::unordered_map<std::pair<const Stop *, const Stop *>, int, StopPtrPairHasher> stopptrpair_to_distance;
    };
}
