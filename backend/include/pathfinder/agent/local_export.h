#pragma once

#include "pathfinder/agent/debug_export.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <cstddef>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentExportFileIdTag {};
using AgentExportFileId = pathfinder::foundation::StrongId<AgentExportFileIdTag>;

struct AgentExportWriteIdTag {};
using AgentExportWriteId = pathfinder::foundation::StrongId<AgentExportWriteIdTag>;

struct AgentExportVerifyIdTag {};
using AgentExportVerifyId = pathfinder::foundation::StrongId<AgentExportVerifyIdTag>;

// --- ID Helpers ---

AgentExportFileId makeAgentExportFileId(
    const std::string& export_id,
    const std::string& kind,
    size_t index);

AgentExportWriteId makeAgentExportWriteId(
    const std::string& manifest_export_id,
    size_t file_count);

AgentExportVerifyId makeAgentExportVerifyId(
    const std::string& write_id);

// --- Enums ---

enum class AgentExportFileKind {
    Unknown,
    Manifest,
    Chunk,
    Diagnostics
};

std::string toString(AgentExportFileKind kind);
pathfinder::foundation::Result<AgentExportFileKind> agentExportFileKindFromString(const std::string& str);

enum class AgentExportFileExtension {
    Unknown,
    Txt,
    Md
};

std::string toString(AgentExportFileExtension ext);
pathfinder::foundation::Result<AgentExportFileExtension> agentExportFileExtensionFromString(const std::string& str);

enum class AgentExportWriteStatus {
    Unknown,
    Planned,
    Written,
    Failed,
    Skipped
};

std::string toString(AgentExportWriteStatus status);
pathfinder::foundation::Result<AgentExportWriteStatus> agentExportWriteStatusFromString(const std::string& str);

enum class AgentExportVerificationStatus {
    Unknown,
    Passed,
    Failed,
    Warning
};

std::string toString(AgentExportVerificationStatus status);
pathfinder::foundation::Result<AgentExportVerificationStatus> agentExportVerificationStatusFromString(const std::string& str);

// --- Data Contracts ---

struct AgentExportPathPolicy {
    std::string root_dir;
    bool allow_create_directories = true;
    bool allow_overwrite = false;
    size_t max_file_count = 64;
    size_t max_file_bytes = 1024 * 1024;
    size_t max_total_bytes = 8 * 1024 * 1024;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportFilePlan {
    AgentExportFileId file_id;
    AgentExportFileKind kind = AgentExportFileKind::Unknown;
    AgentExportFileExtension extension = AgentExportFileExtension::Unknown;
    std::string relative_path;
    std::string content;
    size_t byte_size = 0;
    bool overwrite = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportWritePlan {
    AgentExportWriteId write_id;
    std::string export_id;
    AgentExportPathPolicy path_policy;
    std::vector<AgentExportFilePlan> files;
    size_t total_bytes = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportFileWriteResult {
    AgentExportFileId file_id;
    AgentExportFileKind kind = AgentExportFileKind::Unknown;
    AgentExportWriteStatus status = AgentExportWriteStatus::Unknown;
    std::string relative_path;
    size_t byte_size = 0;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportWriteResult {
    AgentExportWriteId write_id;
    AgentExportWriteStatus status = AgentExportWriteStatus::Unknown;
    std::string root_dir;
    size_t planned_file_count = 0;
    size_t written_file_count = 0;
    size_t total_bytes = 0;
    std::vector<AgentExportFileWriteResult> files;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportVerificationReport {
    AgentExportVerifyId verify_id;
    AgentExportWriteId write_id;
    AgentExportVerificationStatus status = AgentExportVerificationStatus::Unknown;
    size_t expected_file_count = 0;
    size_t actual_file_count = 0;
    size_t expected_total_bytes = 0;
    size_t actual_total_bytes = 0;
    std::vector<std::string> checked_relative_paths;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Write Plan Request & Planner ---

struct AgentExportWritePlanRequest {
    AgentExportDraft draft;
    AgentExportPathPolicy path_policy;
    std::string base_name = "agent_export";
    bool include_manifest_file = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentExportWritePlanner {
public:
    pathfinder::foundation::Result<AgentExportWritePlan> buildPlan(
        const AgentExportWritePlanRequest& request) const;
};

// --- Local Export Service ---

class AgentLocalExportService {
public:
    pathfinder::foundation::Result<AgentExportWriteResult> write(
        const AgentExportWritePlan& plan) const;
};

// --- Verification ---

struct AgentExportVerifyRequest {
    AgentExportWritePlan plan;
    AgentExportWriteResult write_result;
    bool scan_content_for_forbidden_terms = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentExportVerifier {
public:
    pathfinder::foundation::Result<AgentExportVerificationReport> verify(
        const AgentExportVerifyRequest& request) const;
};

} // namespace pathfinder::agent
