#include "transport_catalogue.h"
#include "geo.h"

namespace transport {

    void Catalogue::AddStop(std::string name, Coordinates coordinates) {
        stops_.push_back({std::move(name), std::move(coordinates)});
        Stop& ref = *(stops_.end() - 1);
        stopname_to_stop_[ref.name] = &ref;
        stop_to_buses_names[&ref];
    }

    void Catalogue::SetStopsDistance(std::string_view stop_name_from, std::string_view stop_name_to, int distance) {
        std::pair<const Stop*, const Stop*> stop_ptr_pair{stopname_to_stop_.at(stop_name_from), stopname_to_stop_.at(stop_name_to)};
        stopptrpair_to_distance[stop_ptr_pair] = distance;
    }

    void Catalogue::AddBus(std::string name, const std::vector<std::string_view>& stops_names, bool is_roundtrip) {
        buses_.push_back({std::move(name), is_roundtrip, {}});
        Bus& ref = *(buses_.end() - 1);

        ref.bus_stops.reserve(stops_names.size());
        for (const auto& stop_name : stops_names) {
            const Stop* stop_ptr = stopname_to_stop_.at(stop_name);
            ref.bus_stops.push_back(stop_ptr);
            stop_to_buses_names[stop_ptr].insert(ref.name);
        }

        busname_to_bus_[ref.name] = &ref;
    }

    std::optional<BusInfo> Catalogue::FindBus(std::string_view name_view) const {
        if (!busname_to_bus_.count(name_view)) {
            return {};
        }

        const Bus* bus_ptr = busname_to_bus_.at(name_view);
        std::set<const Stop*> unique_stops_set;
        for (auto it = bus_ptr->bus_stops.begin(); it != bus_ptr->bus_stops.end(); ++it) {
            unique_stops_set.insert(*it);
        }

        int route_length = 0;
        double geo_length = 0.0;
        for (auto it = bus_ptr->bus_stops.begin(); it != bus_ptr->bus_stops.end() - 1; ++it) {
            geo_length += ComputeDistance((*it)->coordinates, (*(it + 1))->coordinates);
            if (stopptrpair_to_distance.count(std::make_pair(*it, *(it + 1)))) {
                route_length += stopptrpair_to_distance.at(std::make_pair(*it, *(it + 1)));
            }
            else {
                route_length += stopptrpair_to_distance.at(std::make_pair(*(it + 1), *it));
            }
        }

        if (!bus_ptr->is_roundtrip) {
            for (auto it = bus_ptr->bus_stops.rbegin(); it != bus_ptr->bus_stops.rend() - 1; ++it) {
                geo_length += ComputeDistance((*it)->coordinates, (*(it + 1))->coordinates);
                if (stopptrpair_to_distance.count(std::make_pair(*it, *(it + 1)))) {
                    route_length += stopptrpair_to_distance.at(std::make_pair(*it, *(it + 1)));
                }
                else {
                    route_length += stopptrpair_to_distance.at(std::make_pair(*(it + 1), *it));
                }
            }
        }

        int stops_on_route = bus_ptr->is_roundtrip
                ? bus_ptr->bus_stops.size()
                : bus_ptr->bus_stops.size() * 2 - 1;

        double curvature = route_length / geo_length;
        return BusInfo{bus_ptr->name, stops_on_route, int(unique_stops_set.size()), route_length, curvature};
    }

    std::optional<StopInfo> Catalogue::FindStop(std::string_view name_view) const {
        if (!stopname_to_stop_.count(name_view)) {
            return {};
        }

        const Stop* stop_ptr = stopname_to_stop_.at(name_view);
        return StopInfo{stop_ptr->name, stop_to_buses_names.at(stop_ptr)};
    }

    const std::deque<Stop>& Catalogue::GetStops() const {
        return stops_;
    }

    const std::map<std::string_view, const Bus*>& Catalogue::GetBuses() const {
        return busname_to_bus_;
    }

}
