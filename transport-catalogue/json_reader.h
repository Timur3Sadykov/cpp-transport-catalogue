#pragma once
#include "request_handler.h"
#include "json.h"
#include <sstream>

namespace transport
{
    class JsonReader
    {
    public:
        explicit JsonReader(RequestHandler& request_handler);

        void SendJsonRequests(std::istream &input);

        void OutputJsonResponse(std::ostream &out);

    private:
        RequestHandler& request_handler_;
        json::Node::Array response_;
    };
}