#pragma once

#include "pathfinder/client_http/client_http_types.h"
#include <string>

namespace pathfinder::client_http {

class ClientStaticFileService {
public:
    explicit ClientStaticFileService(std::string static_root);

    ClientHttpResponse serve(const std::string& path) const;

private:
    std::string static_root_;

    std::string resolveSafePath(const std::string& path) const;
    static std::string mimeType(const std::string& path);
    static bool isAllowedExtension(const std::string& path);
};

} // namespace pathfinder::client_http
