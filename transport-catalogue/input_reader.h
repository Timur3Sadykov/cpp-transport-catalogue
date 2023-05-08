#pragma once
#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <string_view>
#include <deque>

namespace transport {
    namespace detail {
        std::string ReadLine(std::istream &input);

        int ReadLineWithNumber(std::istream &input);
    }

    namespace input {
        enum class QueryType {
            STOP,
            BUS,
        };

        class Reader {
        public:
            explicit Reader(Catalogue& catalogue);

            void ReadUpdateQueries(std::istream &input);

            void UpdateCatalogue();

        private:
            struct StopUpdateQuery {
                std::string line;
                std::string name;
                double latitude = 0, longitude = 0;
                std::deque<std::pair<std::string_view, int>> stops_distance;
            };

            struct BusUpdateQuery {
                std::string line;
                std::string name;
                std::vector<std::string_view> stops_names;
                bool is_circular = false;
            };

            Catalogue& catalogue_;
            std::deque<StopUpdateQuery> stop_update_queries_;
            std::deque<BusUpdateQuery> bus_update_queries_;

            QueryType GetQueryType(const std::string query_line) const;

            void AddStopUpdateQuery(std::string &&line);

            void AddBusUpdateQuery(std::string &&line);
        };
    }
}
