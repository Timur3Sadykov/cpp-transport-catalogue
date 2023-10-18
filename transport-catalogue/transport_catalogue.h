#pragma once
#include "geo.h"

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
        std::string_view name;
        bool found;
        size_t stops_on_route, unique_stops;
        int route_length;
        double curvature;

    };

    struct StopInfo {
        std::string_view name;
        bool found;
        const std::set<std::string_view>& buses_names;
    };

    class Catalogue {
    public:
        void AddStop(std::string_view name_view, const Coordinates& coordinates);

        void SetStopsDistance(std::string_view stop_name_from, std::string_view stop_name_to, int distance);

        void AddBus(std::string_view name_view, const std::vector<std::string_view>& stops_names, bool is_circular = false);

        BusInfo FindBus(std::string_view name_view) const;
        //Вы оставили комментарий, что лучше не копию данных возвращать, а ссылку.
        //BusInfo и StopInfo структуры, которые используются только для хранения
        //результата запроса(вычисляется только по запросу) и возвращаются как временные объекты.
        //Допустима ли такая реализация? Или лучше сохранять все результаты в контейнер и
        //возвращать ссылки на элементы этого контейнера?
        //Структуры содержат в себе только ссылки и числа.

        StopInfo FindStop(std::string_view name_view) const;

    private:
        struct Stop {
            std::string name;
            Coordinates coordinates;
        };

        struct Bus {
            std::string name;
            std::vector<const Stop*> bus_stops;
        };

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
        std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_names;
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPtrPairHasher> stopptrpair_to_distance;
    };
}
