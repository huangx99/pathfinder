#include <cassert>
#include <iostream>
#include "pathfinder/config/validation.h"

using namespace pathfinder::config;
using namespace pathfinder::foundation;

void run_config_validation_tests() {
    // Test 1: Empty report has no errors
    {
        ConfigValidationReport report;
        assert(!report.hasErrors());
        assert(!report.hasFatal());
        assert(report.issueCount() == 0);
    }

    // Test 2: Warning is not an error
    {
        ConfigValidationReport report;
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Warning,
            ErrorCode::validation_config_invalid,
            "test warning"
        });
        assert(!report.hasErrors());
        assert(!report.hasFatal());
        assert(report.issueCount() == 1);
    }

    // Test 3: Error is an error
    {
        ConfigValidationReport report;
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            ErrorCode::validation_config_invalid,
            "test error"
        });
        assert(report.hasErrors());
        assert(!report.hasFatal());
        assert(report.issueCount() == 1);
    }

    // Test 4: Fatal is both fatal and error
    {
        ConfigValidationReport report;
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Fatal,
            ErrorCode::validation_config_invalid,
            "test fatal"
        });
        assert(report.hasErrors());
        assert(report.hasFatal());
        assert(report.issueCount() == 1);
    }

    // Test 5: Mixed issues - issueCount correct
    {
        ConfigValidationReport report;
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Info,
            ErrorCode::common_unknown,
            "info"
        });
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Warning,
            ErrorCode::validation_config_invalid,
            "warning"
        });
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            ErrorCode::validation_config_invalid,
            "error"
        });
        assert(report.hasErrors());
        assert(!report.hasFatal());
        assert(report.issueCount() == 3);
    }

    // Test 6: Issue fields preserved
    {
        ConfigValidationReport report;
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            ErrorCode::id_invalid_format,
            "bad id",
            "test.json",
            "$.objects[0].id",
            42,
            10
        });
        const auto& issue = report.issues()[0];
        assert(issue.severity == ConfigValidationSeverity::Error);
        assert(issue.code == ErrorCode::id_invalid_format);
        assert(issue.message == "bad id");
        assert(issue.file_path == "test.json");
        assert(issue.json_path == "$.objects[0].id");
        assert(issue.line == 42);
        assert(issue.column == 10);
    }

    // Test 7: toString
    {
        assert(toString(ConfigValidationSeverity::Info) == "info");
        assert(toString(ConfigValidationSeverity::Warning) == "warning");
        assert(toString(ConfigValidationSeverity::Error) == "error");
        assert(toString(ConfigValidationSeverity::Fatal) == "fatal");
    }

    std::cout << "config_validation_tests: all passed" << std::endl;
}
