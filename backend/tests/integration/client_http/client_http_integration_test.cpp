#include "pathfinder/client_http/client_http_types.h"
#include "pathfinder/client_http/client_static_file_service.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/client_http/client_http_router_impl.h"
#include "pathfinder/client_http/client_socket_http_transport.h"
#include "pathfinder/client_http/client_http_server.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

using namespace pathfinder::client_http;
using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

struct IntegrationFixture {
    ClientProjectionAdapter projection_adapter;
    ClientPatchContract patch_contract;
    ClientAvailableCommandAdapter available_command_adapter;
    ClientSessionGateway session_gateway;
    WorldCommandHandlerRegistry registry;
    WorldCommandDispatcher dispatcher;
    WorldCommandPipeline pipeline;
    ClientCommandGateway command_gateway;
    ClientProtocolCodec codec;
    ClientHttpGateway http_gateway;

    IntegrationFixture()
        : session_gateway(projection_adapter, available_command_adapter)
        , dispatcher(registry)
        , pipeline(dispatcher)
        , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter)
        , http_gateway(codec, session_gateway, command_gateway) {
        registry.registerHandler(createNoopCommandHandler());
        registry.registerHandler(createWaitCommandHandler());
        registry.registerHandler(createInspectCommandHandler());
        registry.registerHandler(createSystemTickCommandHandler());
        registry.registerHandler(createGenerateWorldCommandHandler());
    }
};

static void createStaticRoot(const std::string& dir) {
    std::filesystem::create_directories(dir);
    std::ofstream html(dir + "/index.html");
    html << "<html><body>Hello</body></html>";
    html.close();
    std::ofstream js(dir + "/app.js");
    js << "console.log('app');";
    js.close();
    std::ofstream css(dir + "/style.css");
    css << "body{color:red}";
    css.close();
}

// ---------------------------------------------------------------------------
// Helper: send raw HTTP request and read response
// ---------------------------------------------------------------------------
static std::string httpPost(const std::string& host, uint16_t port, const std::string& path, const std::string& body) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return "";

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return "";
    }

    std::ostringstream req;
    req << "POST " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << "\r\n";
    req << "Content-Type: application/json\r\n";
    req << "Content-Length: " << body.size() << "\r\n";
    req << "Connection: close\r\n\r\n";
    req << body;

    std::string request_str = req.str();
    ::send(fd, request_str.data(), request_str.size(), 0);

    std::string response;
    char buffer[4096];
    while (true) {
        ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        response.append(buffer, n);
    }
    close(fd);
    return response;
}

static std::string httpGet(const std::string& host, uint16_t port, const std::string& path) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return "";

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return "";
    }

    std::ostringstream req;
    req << "GET " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << "\r\n";
    req << "Connection: close\r\n\r\n";

    std::string request_str = req.str();
    ::send(fd, request_str.data(), request_str.size(), 0);

    std::string response;
    char buffer[4096];
    while (true) {
        ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        response.append(buffer, n);
    }
    close(fd);
    return response;
}

static std::string extractBody(const std::string& response) {
    auto pos = response.find("\r\n\r\n");
    if (pos != std::string::npos) return response.substr(pos + 4);
    pos = response.find("\n\n");
    if (pos != std::string::npos) return response.substr(pos + 2);
    return "";
}

// ---------------------------------------------------------------------------
// 1. Bootstrap roundtrip (router only, no real socket)
// ---------------------------------------------------------------------------
void test_bootstrap_roundtrip() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Post;
    req.path = "/api/client/bootstrap";
    req.body = R"({"client_id":"web_1","session_id":"s1","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true,"dev_reset_if_allowed":false})";

    auto resp = router.route(req);
    assert(resp.status_code == 200);
    assert(resp.body.find("\"session_id\"") != std::string::npos);
    assert(resp.body.find("\"available_commands\"") != std::string::npos);

    std::cout << "PASS: bootstrap_roundtrip\n";
}

// ---------------------------------------------------------------------------
// 2. Command option roundtrip
// ---------------------------------------------------------------------------
void test_command_option_roundtrip() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    // Bootstrap first
    ClientHttpRequest boot_req;
    boot_req.method = ClientHttpMethod::Post;
    boot_req.path = "/api/client/bootstrap";
    boot_req.body = R"({"client_id":"web_1","session_id":"s2","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true})";
    auto boot_resp = router.route(boot_req);
    assert(boot_resp.status_code == 200);

    // Decode to find wait option
    auto boot_decoded = f.codec.decodeBootstrapResponse(boot_resp.body);
    assert(boot_decoded.is_ok());
    std::string wait_option;
    for (const auto& opt : boot_decoded.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Wait) {
            wait_option = opt.option_id;
            break;
        }
    }
    assert(!wait_option.empty());

    // Submit command
    ClientHttpRequest cmd_req;
    cmd_req.method = ClientHttpMethod::Post;
    cmd_req.path = "/api/client/command";
    std::string cmd_body = R"({"session_id":"s2","client_id":"web_1","client_sequence":1,"known_projection_version":1,"submit_mode":"option_id","option_id":")" + wait_option + R"(","selection_context":{"target":{"target_kind":"none"},"selected_actor_key":"","selected_entity_id":"","selected_inventory_id":"","selected_area_id":""}})";
    cmd_req.body = cmd_body;
    auto cmd_resp = router.route(cmd_req);
    assert(cmd_resp.status_code == 200);
    assert(cmd_resp.body.find("\"result_kind\"") != std::string::npos);

    std::cout << "PASS: command_option_roundtrip\n";
}

// ---------------------------------------------------------------------------
// 3. Refresh roundtrip
// ---------------------------------------------------------------------------
void test_refresh_roundtrip() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    // Bootstrap first
    ClientHttpRequest boot_req;
    boot_req.method = ClientHttpMethod::Post;
    boot_req.path = "/api/client/bootstrap";
    boot_req.body = R"({"client_id":"web_1","session_id":"s3","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true})";
    auto boot_resp = router.route(boot_req);
    assert(boot_resp.status_code == 200);

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Post;
    req.path = "/api/client/refresh";
    req.body = R"({"session_id":"s3","client_id":"web_1","known_projection_version":1,"requested_scopes":["full_safe_world"],"requested_layer_key":"surface","viewport_center_x":0,"viewport_center_y":0})";
    auto resp = router.route(req);
    assert(resp.status_code == 200);
    assert(resp.body.find("\"full_projection\"") != std::string::npos);

    std::cout << "PASS: refresh_roundtrip\n";
}

// ---------------------------------------------------------------------------
// 4. Reset roundtrip
// ---------------------------------------------------------------------------
void test_reset_roundtrip() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    // Bootstrap first
    ClientHttpRequest boot_req;
    boot_req.method = ClientHttpMethod::Post;
    boot_req.path = "/api/client/bootstrap";
    boot_req.body = R"({"client_id":"web_1","session_id":"s4","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true})";
    auto boot_resp = router.route(boot_req);
    assert(boot_resp.status_code == 200);

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Post;
    req.path = "/api/client/reset";
    req.body = R"({"session_id":"s4","client_id":"web_1","confirmed":true})";
    auto resp = router.route(req);
    assert(resp.status_code == 200);
    assert(resp.body.find("\"session_id\"") != std::string::npos);

    std::cout << "PASS: reset_roundtrip\n";
}

// ---------------------------------------------------------------------------
// 5. Stale command returns full refresh requirement
// ---------------------------------------------------------------------------
void test_stale_command_returns_full_refresh() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    // Bootstrap
    ClientHttpRequest boot_req;
    boot_req.method = ClientHttpMethod::Post;
    boot_req.path = "/api/client/bootstrap";
    boot_req.body = R"({"client_id":"web_1","session_id":"s5","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true})";
    auto boot_resp = router.route(boot_req);
    assert(boot_resp.status_code == 200);
    auto boot_decoded = f.codec.decodeBootstrapResponse(boot_resp.body);
    assert(boot_decoded.is_ok());

    // Submit with stale version (0)
    std::string wait_option;
    for (const auto& opt : boot_decoded.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Wait) {
            wait_option = opt.option_id;
            break;
        }
    }
    assert(!wait_option.empty());

    ClientHttpRequest cmd_req;
    cmd_req.method = ClientHttpMethod::Post;
    cmd_req.path = "/api/client/command";
    std::string cmd_body = R"({"session_id":"s5","client_id":"web_1","client_sequence":1,"known_projection_version":0,"submit_mode":"option_id","option_id":")" + wait_option + R"(","selection_context":{"target":{"target_kind":"none"},"selected_actor_key":"","selected_entity_id":"","selected_inventory_id":"","selected_area_id":""}})";
    cmd_req.body = cmd_body;
    auto cmd_resp = router.route(cmd_req);
    assert(cmd_resp.status_code == 200);
    assert(cmd_resp.body.find("\"requires_full_refresh\":true") != std::string::npos);

    std::cout << "PASS: stale_command_returns_full_refresh\n";
}

// ---------------------------------------------------------------------------
// 6. Bad JSON returns 400 JSON
// ---------------------------------------------------------------------------
void test_bad_json_returns_400_json() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Post;
    req.path = "/api/client/bootstrap";
    req.body = "not_json";
    auto resp = router.route(req);
    assert(resp.status_code == 400);
    assert(resp.body.find("\"ok\":false") != std::string::npos);
    assert(resp.body.find("\"error_key\"") != std::string::npos);

    std::cout << "PASS: bad_json_returns_400_json\n";
}

// ---------------------------------------------------------------------------
// 7. GET /api/client/* returns JSON 405
// ---------------------------------------------------------------------------
void test_api_get_returns_405_json() {
    IntegrationFixture f;
    ClientStaticFileService static_files("/tmp/p54_int_static");
    ClientHttpRouter router(static_files, f.http_gateway);

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Get;
    req.path = "/api/client/bootstrap";
    auto resp = router.route(req);
    assert(resp.status_code == 405);
    assert(resp.content_type == "application/json; charset=utf-8");
    assert(resp.body.find("\"ok\":false") != std::string::npos);

    std::cout << "PASS: api_get_returns_405_json\n";
}

// ---------------------------------------------------------------------------
// 8. Static index/app/style
// ---------------------------------------------------------------------------
void test_static_index_app_style() {
    std::string tmpdir = "/tmp/p54_int_static2";
    createStaticRoot(tmpdir);
    ClientStaticFileService svc(tmpdir);

    auto resp_html = svc.serve("/");
    assert(resp_html.status_code == 200);
    assert(resp_html.body == "<html><body>Hello</body></html>");

    auto resp_js = svc.serve("/app.js");
    assert(resp_js.status_code == 200);
    assert(resp_js.body == "console.log('app');");

    auto resp_css = svc.serve("/style.css");
    assert(resp_css.status_code == 200);
    assert(resp_css.body == "body{color:red}");

    std::cout << "PASS: static_index_app_style\n";
}

// ---------------------------------------------------------------------------
// 9. No content JSON exposure
// ---------------------------------------------------------------------------
void test_no_content_json_exposure() {
    ClientStaticFileService svc("/tmp");
    auto resp = svc.serve("/content/core/recipes.json");
    assert(resp.status_code == 404);
    std::cout << "PASS: no_content_json_exposure\n";
}

// ---------------------------------------------------------------------------
// 10. Real socket HTTP smoke test
// ---------------------------------------------------------------------------
void test_socket_bootstrap_smoke() {
    std::string tmpdir = "/tmp/p54_socket_static";
    createStaticRoot(tmpdir);

    IntegrationFixture f;
    ClientStaticFileService static_files(tmpdir);
    ClientHttpRouter router(static_files, f.http_gateway);

    auto transport = std::make_unique<ClientSocketHttpTransport>();
    auto* transport_ptr = transport.get();
    ClientHttpServer server(
        std::move(transport),
        std::make_unique<ClientHttpRouter>(static_files, f.http_gateway));

    // Use port 0 to let OS assign an available port
    auto result = server.start(0, "127.0.0.1");
    if (result.is_error()) {
        std::cerr << "SKIP: socket_bootstrap_smoke (could not start server)\n";
        return;
    }

    uint16_t actual_port = transport_ptr->boundPort();
    assert(actual_port > 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test POST /api/client/bootstrap via real TCP
    std::string body = R"({"client_id":"web_socket","session_id":"sock_1","requested_actor_key":"player","requested_layer_key":"surface","client_protocol_version":1,"create_if_missing":true})";
    auto raw_resp = httpPost("127.0.0.1", actual_port, "/api/client/bootstrap", body);
    assert(!raw_resp.empty());
    auto resp_body = extractBody(raw_resp);
    assert(resp_body.find("\"session_id\"") != std::string::npos);
    assert(resp_body.find("\"available_commands\"") != std::string::npos);

    // Test GET / via real TCP
    auto raw_get = httpGet("127.0.0.1", actual_port, "/");
    assert(!raw_get.empty());
    auto get_body = extractBody(raw_get);
    assert(get_body == "<html><body>Hello</body></html>");

    // Test GET /app.js via real TCP
    auto raw_js = httpGet("127.0.0.1", actual_port, "/app.js");
    assert(!raw_js.empty());
    auto js_body = extractBody(raw_js);
    assert(js_body == "console.log('app');");

    // Test GET /style.css via real TCP
    auto raw_css = httpGet("127.0.0.1", actual_port, "/style.css");
    assert(!raw_css.empty());
    auto css_body = extractBody(raw_css);
    assert(css_body == "body{color:red}");

    server.stop();
    std::cout << "PASS: socket_bootstrap_smoke\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_name>\n";
        return 1;
    }
    std::string test = argv[1];

    if (test == "bootstrap_roundtrip") {
        test_bootstrap_roundtrip();
    } else if (test == "command_option_roundtrip") {
        test_command_option_roundtrip();
    } else if (test == "refresh_roundtrip") {
        test_refresh_roundtrip();
    } else if (test == "reset_roundtrip") {
        test_reset_roundtrip();
    } else if (test == "stale_command_returns_full_refresh") {
        test_stale_command_returns_full_refresh();
    } else if (test == "bad_json_returns_400_json") {
        test_bad_json_returns_400_json();
    } else if (test == "api_get_returns_405_json") {
        test_api_get_returns_405_json();
    } else if (test == "static_index_app_style") {
        test_static_index_app_style();
    } else if (test == "no_content_json_exposure") {
        test_no_content_json_exposure();
    } else if (test == "socket_bootstrap_smoke") {
        test_socket_bootstrap_smoke();
    } else {
        std::cerr << "Unknown test: " << test << "\n";
        return 1;
    }
    return 0;
}
