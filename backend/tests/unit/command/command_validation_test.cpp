#include <cassert>
#include <iostream>
#include "pathfinder/command/validation.h"

using namespace pathfinder::command;
using namespace pathfinder::foundation;

static CommandEnvelope makeValidEnvelope() {
    CommandEnvelope env;
    env.command_id = CommandId("cmd_000001");
    env.source = CommandSource::Player;
    env.issued_tick = Tick(100);

    ActionCommand cmd;
    cmd.action_id = ActionId("eat");
    cmd.actor_id = EntityId("ent_000001");
    cmd.targets.push_back(ActionTarget{
        ActionTargetType::Object,
        TargetId("obj_000001"),
        ActionTargetRole::Primary
    });
    env.payload = cmd;

    return env;
}

void run_command_validation_tests() {
    // Test 1: Valid envelope generates empty error report
    {
        auto env = makeValidEnvelope();
        auto report = validateCommandEnvelopeBasic(env);
        assert(!report.hasErrors());
        assert(report.issueCount() == 0);
    }

    // Test 2: Invalid envelope generates error issue
    {
        auto env = makeValidEnvelope();
        env.command_id = CommandId("");
        auto report = validateCommandEnvelopeBasic(env);
        assert(report.hasErrors());
        assert(report.issueCount() > 0);
    }

    // Test 3: Warning is not error
    {
        CommandValidationReport report;
        report.addIssue(CommandValidationIssue{
            CommandValidationSeverity::Warning,
            ErrorCode::command_invalid_argument,
            "test warning",
            "test.field"
        });
        assert(!report.hasErrors());
        assert(report.issueCount() == 1);
    }

    // Test 4: Error is error
    {
        CommandValidationReport report;
        report.addIssue(CommandValidationIssue{
            CommandValidationSeverity::Error,
            ErrorCode::command_missing_required_field,
            "test error",
            "test.field"
        });
        assert(report.hasErrors());
        assert(report.issueCount() == 1);
    }

    // Test 5: Mixed issues - issueCount correct
    {
        CommandValidationReport report;
        report.addIssue(CommandValidationIssue{
            CommandValidationSeverity::Warning,
            ErrorCode::command_invalid_argument,
            "warning",
            ""
        });
        report.addIssue(CommandValidationIssue{
            CommandValidationSeverity::Error,
            ErrorCode::command_missing_required_field,
            "error",
            ""
        });
        assert(report.hasErrors());
        assert(report.issueCount() == 2);
    }

    // Test 6: toString
    {
        assert(toString(CommandValidationSeverity::Warning) == "warning");
        assert(toString(CommandValidationSeverity::Error) == "error");
    }

    // Test 7: Invalid payload propagates
    {
        auto env = makeValidEnvelope();
        env.payload.targets.clear();
        auto report = validateCommandEnvelopeBasic(env);
        assert(report.hasErrors());
    }

    // Test 8: Unknown source generates error
    {
        auto env = makeValidEnvelope();
        env.source = CommandSource::Unknown;
        auto report = validateCommandEnvelopeBasic(env);
        assert(report.hasErrors());
    }

    std::cout << "command_validation_tests: all passed" << std::endl;
}
