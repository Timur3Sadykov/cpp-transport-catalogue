#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"
//#include "tests.h"

int main() {
    //transport::tests::TestTransportCatalogue();

    transport::Catalogue transport_catalogue;
    transport::input::Reader reader(transport_catalogue);
    reader.ReadUpdateQueries(std::cin);
    reader.UpdateCatalogue();

    transport::stat::ReadQueriesToGetStat(std::cin, std::cout, transport_catalogue);

}
