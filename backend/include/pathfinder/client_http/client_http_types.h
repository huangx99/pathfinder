#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace pathfinder::client_http {

enum class ClientHttpMethod {
    Unknown,
    Get,
    Post,
    Options
};

struct ClientHttpRequest {
    ClientHttpMethod method = ClientHttpMethod::Unknown;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct ClientHttpResponse {
    int status_code = 200;
    std::string content_type;
    std::map<std::string, std::string> headers;
    std::string body;
};

} // namespace pathfinder::client_http
