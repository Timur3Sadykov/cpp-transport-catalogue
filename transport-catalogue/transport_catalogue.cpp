#include "transport_catalogue.h"
#include "geo.h"

void transport::Catalogue::AddStop(std::string_view name_view, Coordinates&& coordinates) {
    stops_.push_back(Stop{std::string{name_view}, std::move(coordinates)});
    Stop& ref = *(stops_.end() - 1);
    stopname_to_stop_[ref.name] = &ref;
    stop_to_buses_names[&ref];
}

void transport::Catalogue::SetStopsDistance(std::string_view stop_name_from, std::string_view stop_name_to, int distance) {
    std::pair<const Stop*, const Stop*> stop_ptr_pair{stopname_to_stop_.at(stop_name_from), stopname_to_stop_.at(stop_name_to)};
    stopptrpair_to_distance[stop_ptr_pair] = distance;
}

void transport::Catalogue::AddBus(std::string_view name_view, const std::vector<std::string_view>& stops_names, bool is_circular) {
    buses_.push_back(Bus{std::string{name_view}, {}});
    Bus& ref = *(buses_.end() - 1);
    if (is_circular) {
        ref.bus_stops.reserve(stops_names.size());
        for (auto it = stops_names.begin(); it != stops_names.end(); ++it) {
            const Stop* stop_ptr = stopname_to_stop_.at(*it);
            ref.bus_stops.push_back(stop_ptr);
            stop_to_buses_names[stop_ptr].insert(ref.name);
        }
    }
    else {
        ref.bus_stops.reserve(stops_names.size() * 2 - 1);
        for (auto it = stops_names.begin(); it != stops_names.end(); ++it) {
            const Stop* stop_ptr = stopname_to_stop_.at(*it);
            ref.bus_stops.push_back(stop_ptr);
            stop_to_buses_names[stop_ptr].insert(ref.name);
        }
        for (auto it = stops_names.rbegin() + 1; it != stops_names.rend(); ++it) {
            ref.bus_stops.push_back(stopname_to_stop_.at(*it));
        }
    }

    busname_to_bus_[ref.name] = &ref;
}

transport::BusInfo transport::Catalogue::FindBus(std::string_view name_view) const {
    if (!busname_to_bus_.count(name_view)) {
        return BusInfo{name_view, false};
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

    double curvature = route_length / geo_length;

    return BusInfo{bus_ptr->name, true, bus_ptr->bus_stops.size(), unique_stops_set.size(), route_length, curvature};
}

transport::StopInfo transport::Catalogue::FindStop(std::string_view name_view) const {
    if (!stopname_to_stop_.count(name_view)) {
        return StopInfo{name_view, false, {}};
    }
    const Stop* stop_ptr = stopname_to_stop_.at(name_view);
    return StopInfo{stop_ptr->name, true, stop_to_buses_names.at(stop_ptr)};
}
