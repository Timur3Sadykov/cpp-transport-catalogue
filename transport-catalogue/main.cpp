#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_reader.h"

int main()
{
    transport::Catalogue transport_catalogue;
    transport::MapRenderer renderer;
    transport::RequestHandler handler(transport_catalogue, renderer);
    transport::JsonReader reader(handler);

    reader.SendJsonRequests(std::cin);
    reader.OutputJsonResponse(std::cout);
}