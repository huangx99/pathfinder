#include "pathfinder/client_protocol/client_protocol_harness.h"

namespace pathfinder::client_protocol {

using pathfinder::foundation::Result;

ClientProtocolHarness::ClientProtocolHarness(
    ClientSessionGateway& session_gateway,
    ClientCommandGateway& command_gateway,
    const ClientProtocolCodec& codec)
    : session_gateway_(session_gateway)
    , command_gateway_(command_gateway)
    , codec_(codec) {}

Result<ClientBootstrapResponse> ClientProtocolHarness::fakeBootstrap(
    const std::string& client_id,
    const std::string& session_id,
    const std::string& actor_key) {
    ClientBootstrapRequest request;
    request.client_id = client_id;
    request.session_id = session_id;
    request.requested_actor_key = actor_key;
    request.create_if_missing = true;
    return session_gateway_.bootstrap(request);
}

Result<ClientCommandResponse> ClientProtocolHarness::fakeSubmitOption(
    const std::string& session_id,
    const std::string& client_id,
    uint64_t client_sequence,
    uint64_t known_projection_version,
    const std::string& option_id) {
    ClientCommandRequest request;
    request.session_id = session_id;
    request.client_id = client_id;
    request.client_sequence = client_sequence;
    request.known_projection_version = known_projection_version;
    request.submit_mode = ClientSubmitMode::OptionId;
    request.option_id = option_id;
    return command_gateway_.handleCommand(request);
}

Result<ClientCommandResponse> ClientProtocolHarness::fakeSubmitCommand(
    const std::string& session_id,
    const std::string& client_id,
    uint64_t client_sequence,
    uint64_t known_projection_version,
    const WorldCommandDto& command) {
    ClientCommandRequest request;
    request.session_id = session_id;
    request.client_id = client_id;
    request.client_sequence = client_sequence;
    request.known_projection_version = known_projection_version;
    request.submit_mode = ClientSubmitMode::WorldCommandDto;
    request.command = command;
    return command_gateway_.handleCommand(request);
}

Result<ClientRefreshResponse> ClientProtocolHarness::fakeRefresh(
    const std::string& session_id,
    const std::string& client_id,
    uint64_t known_projection_version) {
    ClientRefreshRequest request;
    request.session_id = session_id;
    request.client_id = client_id;
    request.known_projection_version = known_projection_version;
    return session_gateway_.refresh(request);
}

Result<ClientResetResponse> ClientProtocolHarness::fakeReset(
    const std::string& session_id,
    const std::string& client_id) {
    ClientResetRequest request;
    request.session_id = session_id;
    request.client_id = client_id;
    request.confirmed = true;
    return session_gateway_.reset(request);
}

Result<ClientBootstrapResponse> ClientProtocolHarness::roundtripBootstrap(
    const ClientBootstrapRequest& request) {
    auto encoded = codec_.encodeBootstrapRequest(request);
    if (encoded.is_error()) return Result<ClientBootstrapResponse>::fail(encoded.errors());
    auto decoded = codec_.decodeBootstrapRequest(encoded.value());
    if (decoded.is_error()) return Result<ClientBootstrapResponse>::fail(decoded.errors());
    auto response = session_gateway_.bootstrap(decoded.value());
    if (response.is_error()) return response;
    auto enc_resp = codec_.encodeBootstrapResponse(response.value());
    if (enc_resp.is_error()) return Result<ClientBootstrapResponse>::fail(enc_resp.errors());
    auto dec_resp = codec_.decodeBootstrapResponse(enc_resp.value());
    if (dec_resp.is_error()) return Result<ClientBootstrapResponse>::fail(dec_resp.errors());
    return dec_resp;
}

Result<ClientCommandResponse> ClientProtocolHarness::roundtripCommand(
    const ClientCommandRequest& request) {
    auto encoded = codec_.encodeCommandRequest(request);
    if (encoded.is_error()) return Result<ClientCommandResponse>::fail(encoded.errors());
    auto decoded = codec_.decodeCommandRequest(encoded.value());
    if (decoded.is_error()) return Result<ClientCommandResponse>::fail(decoded.errors());
    auto response = command_gateway_.handleCommand(decoded.value());
    if (response.is_error()) return response;
    auto enc_resp = codec_.encodeCommandResponse(response.value());
    if (enc_resp.is_error()) return Result<ClientCommandResponse>::fail(enc_resp.errors());
    auto dec_resp = codec_.decodeCommandResponse(enc_resp.value());
    if (dec_resp.is_error()) return Result<ClientCommandResponse>::fail(dec_resp.errors());
    return dec_resp;
}

} // namespace pathfinder::client_protocol
