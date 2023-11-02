#include "json_reader.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "domain.h"
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <string_view>

using namespace transport;

namespace
{
    using namespace std::literals;

    const std::string KEY_BASE_R{"base_requests"s};
    const std::string KEY_STAT_R{"stat_requests"s};
    const std::string KEY_RENDER_S{"render_settings"s};
    const std::string KEY_STOP{"Stop"s};
    const std::string KEY_BUS{"Bus"s};
    const std::string KEY_TYPE{"type"s};
    const std::string KEY_NAME{"name"s};
    const std::string KEY_MAP_REQ{"Map"s};
    const std::string KEY_MAP_RESP{"map"s};
    const std::string KEY_LATITUDE{"latitude"s};
    const std::string KEY_LONGITUDE{"longitude"s};
    const std::string KEY_R_DISTANCES{"road_distances"s};
    const std::string KEY_ROUNDTRIP{"is_roundtrip"s};
    const std::string KEY_ID{"id"s};
    const std::string KEY_CURVATURE{"curvature"s};
    const std::string KEY_REQUEST_ID{"request_id"s};
    const std::string KEY_R_LENGTH{"route_length"s};
    const std::string KEY_STOP_COUNT{"stop_count"s};
    const std::string KEY_U_STOP_COUNT{"unique_stop_count"s};
    const std::string KEY_BUSES{"buses"s};
    const std::string KEY_STOPS{"stops"s};
    const std::string KEY_ERROR{"error_message"s};
    const std::string NOT_FOUND{"not found"s};
    const std::string KEY_WIDTH{"width"s};
    const std::string KEY_HEIGHT{"height"s};
    const std::string KEY_PADDING{"padding"s};
    const std::string KEY_LINE_WIDTH{"line_width"s};
    const std::string KEY_STOP_R{"stop_radius"s};
    const std::string KEY_UNDERL_WIDTH{"underlayer_width"s};
    const std::string KEY_BUS_LABEL_SIZE{"bus_label_font_size"s};
    const std::string KEY_STOP_LABEL_SIZE{"stop_label_font_size"s};
    const std::string KEY_BUS_LABEL_OFFSET{"bus_label_offset"s};
    const std::string KEY_STOP_LABEL_OFFSET{"stop_label_offset"s};
    const std::string KEY_UNDERL_COLOR{"underlayer_color"s};
    const std::string KEY_COLOR_PALETTE{"color_palette"s};

    svg::Color GetColor(const json::Node& color_node)
    {
        if (color_node.IsString())
        {
            return color_node.AsString();
        }
        else
        {
            const auto& color_array = color_node.AsArray();

            if (color_array.size() == 3u)
            {
                return svg::Rgb(color_array[0].AsInt(), color_array[1].AsInt(), color_array[2].AsInt());
            }
            else
            {
                return svg::Rgba(color_array[0].AsInt(), color_array[1].AsInt(), color_array[2].AsInt(), color_array[3].AsDouble());
            }
        }
    }

    void SendBaseRequests(transport::RequestHandler& request_handler, const json::Node& requests)
    {
        for (const auto& request : requests.AsArray())
        {
            const std::string& request_key = request.At(KEY_TYPE).AsString();

            if (request_key == KEY_STOP)
            {
                std::map<std::string_view, int> road_distances;

                for (const auto& [stop_name, distance] : request.At(KEY_R_DISTANCES).AsObject())
                {
                    road_distances.emplace(stop_name, distance.AsInt());
                }

                request_handler.AddStopRequest(
                        request.At(KEY_NAME).AsString(),
                        {request.At(KEY_LATITUDE).AsDouble(), request.At(KEY_LONGITUDE).AsDouble()},
                        road_distances);
            }
            else if (request_key == KEY_BUS)
            {
                std::vector<std::string_view> stops;
                const auto& stops_array = request.At(KEY_STOPS).AsArray();
                stops.reserve(stops_array.size());

                for (const auto& stop_name : stops_array)
                {
                    stops.push_back(stop_name.AsString());
                }

                request_handler.AddBusRequest(
                        request.At(KEY_NAME).AsString(),
                        stops,
                        request.At(KEY_ROUNDTRIP).AsBool());
            }
        }

        request_handler.UpdateCatalogue();
    }

    void SendRenderSettings(transport::RequestHandler& request_handler, const json::Node& requests)
    {
        transport::RenderSettings settings{};

        if (requests.Contains(KEY_WIDTH))
        {
            settings.width = requests.At(KEY_WIDTH).AsDouble();
        }
        if (requests.Contains(KEY_HEIGHT))
        {
            settings.height = requests.At(KEY_HEIGHT).AsDouble();
        }
        if (requests.Contains(KEY_PADDING))
        {
            settings.padding = requests.At(KEY_PADDING).AsDouble();
        }
        if (requests.Contains(KEY_LINE_WIDTH))
        {
            settings.line_width = requests.At(KEY_LINE_WIDTH).AsDouble();
        }
        if (requests.Contains(KEY_STOP_R))
        {
            settings.stop_radius = requests.At(KEY_STOP_R).AsDouble();
        }
        if (requests.Contains(KEY_UNDERL_WIDTH))
        {
            settings.underlayer_width = requests.At(KEY_UNDERL_WIDTH).AsDouble();
        }
        if (requests.Contains(KEY_BUS_LABEL_SIZE))
        {
            settings.bus_label_font_size = requests.At(KEY_BUS_LABEL_SIZE).AsInt();
        }
        if (requests.Contains(KEY_STOP_LABEL_SIZE))
        {
            settings.stop_label_font_size = requests.At(KEY_STOP_LABEL_SIZE).AsInt();
        }
        if (requests.Contains(KEY_BUS_LABEL_OFFSET))
        {
            const auto& array = requests.At(KEY_BUS_LABEL_OFFSET).AsArray();
            settings.bus_label_offset = {array[0].AsDouble(), array[1].AsDouble()};
        }
        if (requests.Contains(KEY_STOP_LABEL_OFFSET))
        {
            const auto& array = requests.At(KEY_STOP_LABEL_OFFSET).AsArray();
            settings.stop_label_offset = {array[0].AsDouble(), array[1].AsDouble()};
        }
        if (requests.Contains(KEY_UNDERL_COLOR))
        {
            settings.underlayer_color = GetColor(requests.At(KEY_UNDERL_COLOR));
        }
        if (requests.Contains(KEY_COLOR_PALETTE))
        {
            const auto& palette_array = requests.At(KEY_COLOR_PALETTE).AsArray();

            for (const auto& color_node : palette_array)
            {
                settings.color_palette.push_back(GetColor(color_node));
            }
        }

        request_handler.SetRendererSettings(settings);
    }

    json::Node::Object SendStopStatRequest(const transport::RequestHandler& request_handler, const json::Node& request)
    {
        auto object_builder = json::Builder{};
        object_builder.StartObject();
        auto stop_info = request_handler.GetStopInfo(request.At(KEY_NAME).AsString());

        if (stop_info)
        {
            auto array_builder = json::Builder{};
            array_builder.StartArray();

            for (const auto& bus_name: stop_info->buses_names)
            {
                array_builder.Value(std::string{bus_name});
            }

            array_builder.EndArray();

            object_builder.Key(KEY_BUSES).Value(array_builder.Build().AsArray());
            object_builder.Key(KEY_REQUEST_ID).Value(request.At(KEY_ID).AsInt());
        }
        else
        {
            object_builder.Key(KEY_REQUEST_ID).Value(request.At(KEY_ID).AsInt());
            object_builder.Key(KEY_ERROR).Value(NOT_FOUND);
        }

        object_builder.EndObject();
        return object_builder.Build().AsObject();
    }

    json::Node::Object SendBusStatRequest(const transport::RequestHandler& request_handler, const json::Node& request)
    {
        auto object_builder = json::Builder{};
        object_builder.StartObject();
        auto bus_info = request_handler.GetBusInfo(request.At(KEY_NAME).AsString());

        if (bus_info)
        {
            object_builder.Key(KEY_CURVATURE).Value(bus_info->curvature);
            object_builder.Key(KEY_REQUEST_ID).Value(request.At(KEY_ID).AsInt());
            object_builder.Key(KEY_R_LENGTH).Value(bus_info->route_length);
            object_builder.Key(KEY_STOP_COUNT).Value(bus_info->stops_on_route);
            object_builder.Key(KEY_U_STOP_COUNT).Value(bus_info->unique_stops);
        }
        else
        {
            object_builder.Key(KEY_REQUEST_ID).Value(request.At(KEY_ID).AsInt());
            object_builder.Key(KEY_ERROR).Value(NOT_FOUND);
        }

        object_builder.EndObject();
        return object_builder.Build().AsObject();
    }

    json::Node::Object SendMapStatRequest(const transport::RequestHandler& request_handler, const json::Node& request)
    {
        std::ostringstream out;
        request_handler.RenderMap().Render(out);

        auto object_builder = json::Builder{};
        object_builder.StartObject();
        object_builder.Key(KEY_MAP_RESP).Value(out.str());
        object_builder.Key(KEY_REQUEST_ID).Value(request.At(KEY_ID).AsInt());
        object_builder.EndObject();

        return object_builder.Build().AsObject();
    }

    json::Node::Array SendStatRequests(const transport::RequestHandler& request_handler, const json::Node& requests)
    {
        auto array_builder = json::Builder{};
        array_builder.StartArray();

        try
        {
            for (const auto& request: requests.AsArray())
            {
                const std::string& request_key = request.At(KEY_TYPE).AsString();

                if (request_key == KEY_STOP)
                {
                    array_builder.Value(SendStopStatRequest(request_handler, request));
                }
                else if (request_key == KEY_BUS)
                {
                    array_builder.Value(SendBusStatRequest(request_handler, request));
                }
                else if (request_key == KEY_MAP_REQ)
                {
                    array_builder.Value(SendMapStatRequest(request_handler, request));
                }
            }
        }
        catch (const json::JsonException& e)
        {}

        array_builder.EndArray();
        return array_builder.Build().AsArray();
    }
}

JsonReader::JsonReader(RequestHandler& request_handler)
    : request_handler_(request_handler)
{
    response_builder_.StartArray();
}

void JsonReader::SendJsonRequests(std::istream& input)
{
    const auto json_requests = json::Load(input).GetRoot();

    if (json_requests.Contains(KEY_BASE_R))
    {
        SendBaseRequests(request_handler_, json_requests.At(KEY_BASE_R));
    }

    if (json_requests.Contains(KEY_RENDER_S))
    {
        SendRenderSettings(request_handler_, json_requests.At(KEY_RENDER_S));
    }

    if (json_requests.Contains(KEY_STAT_R))
    {
        response_builder_.Merge(SendStatRequests(request_handler_, json_requests.At(KEY_STAT_R)));
    }
}

void JsonReader::OutputJsonResponse(std::ostream& out)
{
    response_builder_.EndArray();
    json::Print(json::Document(response_builder_.Build()), out);
    response_builder_.Clear();
}