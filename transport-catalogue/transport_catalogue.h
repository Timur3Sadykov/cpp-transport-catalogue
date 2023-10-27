#pragma once
#include "domain.h"
#include "geo.h"
#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <set>
#include <optional>

namespace transport {

    class Catalogue {
    public:
        void AddStop(std::string name, Coordinates coordinates);

        void SetStopsDistance(std::string_view stop_name_from, std::string_view stop_name_to, int distance);

        void AddBus(std::string name, const std::vector<std::string_view>& stops_names, bool is_roundtrip = false);

        std::optional<BusInfo> FindBus(std::string_view name_view) const;

        std::optional<StopInfo> FindStop(std::string_view name_view) const;

        const std::deque<Stop>& GetStops() const;

        const std::map<std::string_view, const Bus*>& GetBuses() const;

    private:
        class StopPtrPairHasher {
        public:
            size_t operator()(const std::pair<const Stop*, const Stop*> &pair) const {
                size_t h_first = hasher_(pair.first);
                size_t h_second = hasher_(pair.second);

                return h_first + h_second * 37;
            }

        private:
            std::hash<const Stop*> hasher_;
        };

        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
        std::deque<Bus> buses_;
        std::map<std::string_view, const Bus*> busname_to_bus_;
        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_names;
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPtrPairHasher> stopptrpair_to_distance;
    };

}