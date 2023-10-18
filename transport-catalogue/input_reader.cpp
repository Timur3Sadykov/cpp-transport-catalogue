#include "input_reader.h"

std::string transport::detail::ReadLine(std::istream& input) {
    std::string s;
    getline(input, s);
    return s;
}

int transport::detail::ReadLineWithNumber(std::istream& input) {
    int result;
    input >> result;
    ReadLine(input);
    return result;
}

transport::input::Reader::Reader(Catalogue& catalogue)
            : catalogue_(catalogue) {}

void transport::input::Reader::ReadQueries(std::istream& input) {
    int n = detail::ReadLineWithNumber(input);
    for (int i = 0; i < n; ++i) {
        std::string query_line = detail::ReadLine(input);
        if (GetQueryType(query_line) == QueryType::STOP) {
            AddStopQuery(std::move(query_line));
        }
        else {
            AddBusQuery(std::move(query_line));
        }
    }
}

void transport::input::Reader::UpdateCatalogue() {
    for (const StopUpdateQuery& query : stop_update_queries_) {
        catalogue_.AddStop(query.name, query.coordinates);
    }
    for (const StopUpdateQuery& query : stop_update_queries_) {
        for (const auto& [stop_name_to, distance] : query.stops_distance) {
            catalogue_.SetStopsDistance(query.name, stop_name_to, distance);
        }
    }

    for (const BusUpdateQuery& query : bus_update_queries_) {
        catalogue_.AddBus(query.name, query.stops_names, query.is_circular);
    }
}

transport::input::QueryType transport::input::Reader::GetQueryType(const std::string query_line) const {
    if (query_line.substr(0, 3) == "Bus"s) {
        return QueryType::BUS;
    }
    else {
        return QueryType::STOP;
    }
}

void transport::input::Reader::AddStopQuery(std::string&& line) {
    stop_update_queries_.resize(stop_update_queries_.size() + 1);
    StopUpdateQuery& query = *(stop_update_queries_.end() - 1);
    query.line = std::move(line);

    auto start_name = query.line.find_first_not_of(' ', 4);
    auto stop_name = query.line.find(':', start_name);
    query.name = query.line.substr(start_name, stop_name - start_name);

    auto start_lat = query.line.find_first_of("0123456789"s, stop_name);
    auto stop_lat = query.line.find(',', start_lat);
    double latitude = std::stod(query.line.substr(start_lat, stop_lat - start_lat));

    auto start_lng = query.line.find_first_of("0123456789"s, stop_lat);
    auto stop_lng = query.line.find(',', start_lng);
    double longitude = 0.0;
    if (stop_lng == query.line.npos) {
        longitude = std::stod(query.line.substr(start_lng));
    }
    else {
        longitude = std::stod(query.line.substr(start_lng, stop_lng-start_lng));

        std::string_view line_view{query.line};
        line_view.remove_prefix(line_view.find_first_not_of(' ', stop_lng + 1));
        const int64_t pos_end = line_view.npos;
        while (!line_view.empty()) {
            auto m_border = line_view.find('m') + 5;
            auto border = line_view.find(',');
            if (border != pos_end) {
                query.stops_distance.push_back({line_view.substr(m_border, border - m_border), std::stoi(std::string((line_view.substr(0, m_border))))});
                line_view.remove_prefix(std::min(line_view.find_first_not_of(' ', border + 1), line_view.size()));
            }
            else {
                query.stops_distance.push_back({line_view.substr(m_border), std::stoi(std::string((line_view.substr(0, m_border))))});
                break;
            }
        }
    }
    query.coordinates = Coordinates{latitude, longitude};
 }

 void transport::input::Reader::AddBusQuery(std::string&& line) {
    bus_update_queries_.resize(bus_update_queries_.size() + 1);
    BusUpdateQuery& query = *(bus_update_queries_.end() - 1);
    query.line = std::move(line);

    auto start_name = query.line.find_first_not_of(' ', 3);
    auto stop_name = query.line.find(':', start_name);
    query.name = query.line.substr(start_name, stop_name - start_name);

    std::string_view line_view{query.line};
    line_view.remove_prefix(line_view.find_first_not_of(' ', stop_name + 1));
    if (line_view[line_view.find_first_of("->"s)] == '>') {
        query.is_circular = true;
    }

    const int64_t pos_end = line_view.npos;
    while (!line_view.empty()) {
        auto border = line_view.find_first_of("->"s);
        if (border != pos_end) {
            query.stops_names.push_back(line_view.substr(0, line_view.find_last_not_of(' ', border) - 1));
            line_view.remove_prefix(std::min(line_view.find_first_not_of(' ', border + 1), line_view.size()));
        }
        else {
            query.stops_names.push_back(line_view);
            break;
        }
    }
}
