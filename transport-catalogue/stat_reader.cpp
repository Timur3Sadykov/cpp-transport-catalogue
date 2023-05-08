#include "stat_reader.h"

std::ostream& transport::stat::operator<<(std::ostream& out, const BusInfo& bus_info) {
    out << "Bus "s << bus_info.name << ": "s;
    if (!bus_info.found) {
        out << "not found"s;
    }
    else {
        out << bus_info.stops_on_route << " stops on route, "s
        << bus_info.unique_stops << " unique stops, "s
        << bus_info.route_length << " route length, "s
        << std::setprecision(6) << bus_info.curvature << " curvature"s;
    }
    return out;
}

std::ostream& transport::stat::operator<<(std::ostream& out, const StopInfo& stop_info) {
    out << "Stop "s << stop_info.name << ": "s;
    if (!stop_info.found) {
        out << "not found"s;
    }
    else if (stop_info.buses_names.empty()) {
        out << "no buses"s;
    } else {
        out << "buses";
        for (std::string_view bus_name: stop_info.buses_names) {
            out << " "s << bus_name;
        }
    }
    return out;
}

transport::BusInfo transport::stat::GetBusInfo(const Catalogue& catalogue, std::string_view query_view) {
    auto start_name = query_view.find_first_not_of(' ', 3);
    auto stop_name = query_view.find_last_not_of(' ') + 1;

    return catalogue.FindBus(query_view.substr(start_name, stop_name - start_name));
}

transport::StopInfo transport::stat::GetStopInfo(const Catalogue& catalogue, std::string_view query_view) {
    auto start_name = query_view.find_first_not_of(' ', 4);
    auto stop_name = query_view.find_last_not_of(' ') + 1;

    return catalogue.FindStop(query_view.substr(start_name, stop_name - start_name));
}

void transport::stat::ReadQueriesToGetStat(std::istream& input, std::ostream& out, Catalogue& catalogue) {
    int n = detail::ReadLineWithNumber(input);
    for (int i = 0; i < n; ++i) {
        std::string query = detail::ReadLine(input);
        if (query.substr(0, 3) == "Bus"s) {
            out << GetBusInfo(catalogue, query) << "\n";
        } else {
            out << GetStopInfo(catalogue, query) << "\n";
        }
    }
}
