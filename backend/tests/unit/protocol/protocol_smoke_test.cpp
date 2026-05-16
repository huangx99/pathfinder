#include "pathfinder/protocol/types.h"
#include "pathfinder/protocol/envelope.h"
#include "pathfinder/protocol/command_result_adapter.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::protocol;
using namespace pathfinder::foundation;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    // --- Smoke ---
    if (test_name == "smoke") {
        assert(protocol_module_version() == 1);
        std::cout << "PASS: protocol smoke" << std::endl;
        return 0;
    }

    // --- ProtocolMessageType ---
    if (test_name == "message_type_tostring") {
        assert(toString(ProtocolMessageType::Request) == "request");
        assert(toString(ProtocolMessageType::Response) == "response");
        assert(toString(ProtocolMessageType::Event) == "event");
        assert(toString(ProtocolMessageType::Ack) == "ack");
        assert(toString(ProtocolMessageType::Heartbeat) == "heartbeat");
        assert(toString(ProtocolMessageType::Error) == "error");
        assert(toString(ProtocolMessageType::Progress) == "progress");
        assert(toString(ProtocolMessageType::Unknown) == "unknown");
        std::cout << "PASS: message type toString" << std::endl;
        return 0;
    }

    if (test_name == "message_type_fromstring") {
        auto r1 = protocolMessageTypeFromString("request");
        assert(r1.is_ok() && r1.value() == ProtocolMessageType::Request);
        auto r2 = protocolMessageTypeFromString("response");
        assert(r2.is_ok() && r2.value() == ProtocolMessageType::Response);
        auto r3 = protocolMessageTypeFromString("invalid");
        assert(r3.is_error());
        auto r4 = protocolMessageTypeFromString("unknown");
        assert(r4.is_error()); // "unknown" should not parse to valid business value
        std::cout << "PASS: message type fromString" << std::endl;
        return 0;
    }

    // --- ProtocolDomain ---
    if (test_name == "domain_tostring") {
        assert(toString(ProtocolDomain::Command) == "command");
        assert(toString(ProtocolDomain::Query) == "query");
        assert(toString(ProtocolDomain::EventStream) == "event_stream");
        assert(toString(ProtocolDomain::Save) == "save");
        assert(toString(ProtocolDomain::Replay) == "replay");
        assert(toString(ProtocolDomain::Health) == "health");
        assert(toString(ProtocolDomain::Unknown) == "unknown");
        std::cout << "PASS: domain toString" << std::endl;
        return 0;
    }

    if (test_name == "domain_fromstring") {
        auto r1 = protocolDomainFromString("command");
        assert(r1.is_ok() && r1.value() == ProtocolDomain::Command);
        auto r2 = protocolDomainFromString("event_stream");
        assert(r2.is_ok() && r2.value() == ProtocolDomain::EventStream);
        auto r3 = protocolDomainFromString("invalid");
        assert(r3.is_error());
        auto r4 = protocolDomainFromString("unknown");
        assert(r4.is_error());
        std::cout << "PASS: domain fromString" << std::endl;
        return 0;
    }

    // --- ProtocolPayloadType ---
    if (test_name == "payload_type_tostring") {
        assert(toString(ProtocolPayloadType::CommandResult) == "command_result");
        assert(toString(ProtocolPayloadType::EventList) == "event_list");
        assert(toString(ProtocolPayloadType::ReplayReport) == "replay_report");
        assert(toString(ProtocolPayloadType::ErrorList) == "error_list");
        assert(toString(ProtocolPayloadType::Text) == "text");
        assert(toString(ProtocolPayloadType::Unknown) == "unknown");
        std::cout << "PASS: payload type toString" << std::endl;
        return 0;
    }

    if (test_name == "payload_type_fromstring") {
        auto r1 = protocolPayloadTypeFromString("command_result");
        assert(r1.is_ok() && r1.value() == ProtocolPayloadType::CommandResult);
        auto r2 = protocolPayloadTypeFromString("error_list");
        assert(r2.is_ok() && r2.value() == ProtocolPayloadType::ErrorList);
        auto r3 = protocolPayloadTypeFromString("invalid");
        assert(r3.is_error());
        auto r4 = protocolPayloadTypeFromString("unknown");
        assert(r4.is_error());
        std::cout << "PASS: payload type fromString" << std::endl;
        return 0;
    }

    // --- ProtocolEnvelope ---
    if (test_name == "envelope_valid_passes") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg_001";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        env.payload.message_key = "command.succeeded";
        auto result = env.validateBasic();
        assert(result.is_ok());
        std::cout << "PASS: valid envelope passes" << std::endl;
        return 0;
    }

    if (test_name == "envelope_missing_version_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "";
        env.message_id = "msg_001";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: missing version fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_missing_message_id_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: missing message_id fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_invalid_message_id_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg with spaces";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: invalid message_id fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_unknown_message_type_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg_001";
        env.message_type = ProtocolMessageType::Unknown;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: unknown message_type fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_unknown_domain_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg_001";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Unknown;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: unknown domain fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_unknown_payload_type_fails") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg_001";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::Unknown;
        auto result = env.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: unknown payload_type fails" << std::endl;
        return 0;
    }

    if (test_name == "envelope_with_errors_passes") {
        ProtocolEnvelope env;
        env.protocol_version = "1.0";
        env.message_id = "msg_002";
        env.message_type = ProtocolMessageType::Response;
        env.domain = ProtocolDomain::Command;
        env.payload.payload_type = ProtocolPayloadType::CommandResult;
        env.payload.message_key = "command.failed";
        ProtocolError err;
        err.code = "rule_error";
        err.message_key = "eat.failed";
        err.debug_text = "Object already consumed";
        env.errors.push_back(err);
        auto result = env.validateBasic();
        assert(result.is_ok());
        std::cout << "PASS: envelope with errors passes" << std::endl;
        return 0;
    }

    // --- CommandResultAdapter ---
    if (test_name == "success_pipeline_to_envelope") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        pathfinder::pipeline::RulePipeline pipeline;
        pathfinder::state::GameState exec_state = initial_state;
        pathfinder::pipeline::PipelineContext ctx;
        ctx.command = cmd;
        ctx.state_metadata = exec_state.metadata;
        ctx.game_state = &exec_state;
        ctx.random_seed = pathfinder::foundation::RandomSeed(42);
        auto result = pipeline.execute(ctx);
        assert(result.is_ok());

        auto env = buildCommandResponseEnvelope(cmd, result.value());
        auto validation = env.validateBasic();
        assert(validation.is_ok());
        assert(env.domain == ProtocolDomain::Command);
        assert(env.message_type == ProtocolMessageType::Response);
        assert(env.payload.payload_type == ProtocolPayloadType::CommandResult);
        assert(env.payload.message_key == "command.succeeded");
        assert(env.state_version.has_value());
        std::cout << "PASS: success pipeline to envelope" << std::endl;
        return 0;
    }

    if (test_name == "failed_pipeline_to_envelope") {
        // Create a failed pipeline result
        pathfinder::pipeline::PipelineResult failed_result;
        failed_result.status = pathfinder::pipeline::PipelineStatus::Failed;
        failed_result.errors.push_back(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::rule_missing_resolver,
                "rule.no_resolver",
                "No resolver found"));

        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();
        auto env = buildCommandResponseEnvelope(cmd, failed_result);
        auto validation = env.validateBasic();
        assert(validation.is_ok());
        assert(env.payload.message_key == "command.failed");
        assert(env.errors.size() == 1);
        assert(env.errors[0].message_key == "rule.no_resolver");
        std::cout << "PASS: failed pipeline to envelope" << std::endl;
        return 0;
    }

    if (test_name == "envelope_no_state_change_draft") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        pathfinder::pipeline::RulePipeline pipeline;
        pathfinder::state::GameState exec_state = initial_state;
        pathfinder::pipeline::PipelineContext ctx;
        ctx.command = cmd;
        ctx.state_metadata = exec_state.metadata;
        ctx.game_state = &exec_state;
        ctx.random_seed = pathfinder::foundation::RandomSeed(42);
        auto result = pipeline.execute(ctx);
        assert(result.is_ok());

        auto env = buildCommandResponseEnvelope(cmd, result.value());
        // Verify the envelope doesn't expose internal state draft structures
        // The payload is a simple string summary, not a serialized state draft
        assert(env.payload.payload_type == ProtocolPayloadType::CommandResult);
        assert(!env.payload.debug_text.empty());
        // debug_text should contain counts, not internal structures
        assert(env.payload.debug_text.find("changes=") != std::string::npos);
        std::cout << "PASS: envelope no state change draft" << std::endl;
        return 0;
    }

    return 1;
}
