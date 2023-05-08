#pragma once
#include "transport_catalogue.h"
#include "input_reader.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <string_view>

namespace transport {
    namespace stat {
        std::ostream& operator<<(std::ostream& out, const BusInfo& bus_info);

        std::ostream& operator<<(std::ostream& out, const StopInfo& stop_info);

        BusInfo GetBusInfo(const Catalogue& catalogue, std::string_view query_view);

        StopInfo GetStopInfo(const Catalogue& catalogue, std::string_view query_view);

        void ReadQueriesToGetStat(std::istream& input, std::ostream& out, Catalogue& catalogue);
    }
}
