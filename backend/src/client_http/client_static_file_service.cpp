#include "pathfinder/client_http/client_static_file_service.h"
#include <fstream>
#include <sstream>

namespace pathfinder::client_http {

namespace {

std::string readFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // namespace

ClientStaticFileService::ClientStaticFileService(std::string static_root)
    : static_root_(std::move(static_root)) {
    if (!static_root_.empty() && static_root_.back() == '/') {
        static_root_.pop_back();
    }
}

ClientHttpResponse ClientStaticFileService::serve(const std::string& path) const {
    auto safe_path = resolveSafePath(path);
    if (safe_path.empty()) {
        return ClientHttpResponse{
            404,
            "text/plain; charset=utf-8",
            {},
            "Not Found"
        };
    }

    auto content = readFileBinary(safe_path);
    if (content.empty()) {
        return ClientHttpResponse{
            404,
            "text/plain; charset=utf-8",
            {},
            "Not Found"
        };
    }

    return ClientHttpResponse{
        200,
        mimeType(safe_path),
        {{"Cache-Control", "no-store, max-age=0"}},
        content
    };
}

std::string ClientStaticFileService::resolveSafePath(const std::string& path) const {
    if (path.empty()) return "";
    if (path.find("..") != std::string::npos) return "";
    if (path.find("content/core") != std::string::npos) return "";
    if (path.find("backend/") != std::string::npos) return "";
    if (path.find("doc/") != std::string::npos) return "";
    if (path.find(".git") != std::string::npos) return "";

    std::string resolved = path;
    while (!resolved.empty() && resolved.front() == '/') {
        resolved = resolved.substr(1);
    }
    if (resolved.empty()) resolved = "index.html";

    if (!isAllowedExtension(resolved)) return "";

    return static_root_ + "/" + resolved;
}

std::string ClientStaticFileService::mimeType(const std::string& path) {
    if (path.ends_with(".html")) return "text/html; charset=utf-8";
    if (path.ends_with(".css")) return "text/css; charset=utf-8";
    if (path.ends_with(".js")) return "application/javascript; charset=utf-8";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".webp")) return "image/webp";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".ico")) return "image/x-icon";
    if (path.ends_with(".json")) return "application/json; charset=utf-8";
    return "application/octet-stream";
}

bool ClientStaticFileService::isAllowedExtension(const std::string& path) {
    if (path.ends_with(".html")) return true;
    if (path.ends_with(".css")) return true;
    if (path.ends_with(".js")) return true;
    if (path.ends_with(".png")) return true;
    if (path.ends_with(".jpg")) return true;
    if (path.ends_with(".jpeg")) return true;
    if (path.ends_with(".webp")) return true;
    if (path.ends_with(".svg")) return true;
    if (path.ends_with(".ico")) return true;
    // .json only allowed for client manifest files
    if (path.ends_with("manifest.json")) return true;
    return false;
}

} // namespace pathfinder::client_http
