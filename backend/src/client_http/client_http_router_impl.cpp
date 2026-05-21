#include "pathfinder/client_http/client_http_router_impl.h"
#include "pathfinder/client_http/client_static_file_service.h"
#include "pathfinder/client_http/client_http_gateway.h"

namespace pathfinder::client_http {

ClientHttpRouter::ClientHttpRouter(
    ClientStaticFileService& static_files,
    ClientHttpGateway& gateway)
    : static_files_(static_files)
    , gateway_(gateway) {
}

ClientHttpResponse ClientHttpRouter::route(const ClientHttpRequest& request) {
    // API paths: must return JSON for all error cases
    if (request.path.starts_with("/api/client/")) {
        if (request.method == ClientHttpMethod::Options) {
            return ClientHttpResponse{
                200,
                "text/plain; charset=utf-8",
                {{"Access-Control-Allow-Origin", "*"},
                 {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                 {"Access-Control-Allow-Headers", "Content-Type"}},
                "OK"
            };
        }
        if (request.method == ClientHttpMethod::Post) {
            return gateway_.handleApi(request);
        }
        return ClientHttpResponse{
            405,
            "application/json; charset=utf-8",
            {},
            "{\"ok\":false,\"error_key\":\"method_not_allowed\",\"message\":\"Only POST is allowed for API endpoints\",\"details\":[]}"};
    }

    // Non-API paths
    if (request.method == ClientHttpMethod::Options) {
        return ClientHttpResponse{
            200,
            "text/plain; charset=utf-8",
            {{"Access-Control-Allow-Origin", "*"},
             {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
             {"Access-Control-Allow-Headers", "Content-Type"}},
            "OK"
        };
    }

    if (request.method == ClientHttpMethod::Get) {
        return static_files_.serve(request.path);
    }

    return ClientHttpResponse{
        405,
        "text/plain; charset=utf-8",
        {},
        "Method Not Allowed"
    };
}

} // namespace pathfinder::client_http
