#include "pathfinder/agent/local_export.h"
#include <algorithm>
#include <set>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace pathfinder::agent {

namespace {

bool isValidRelativePathComponent(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == '/')) {
            return false;
        }
    }
    return true;
}

bool isValidBaseName(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

// --- ID Helpers ---

AgentExportFileId makeAgentExportFileId(
    const std::string& export_id,
    const std::string& kind,
    size_t index) {
    return AgentExportFileId("agent_export_file_" + export_id + "_" + kind + "_" + std::to_string(index));
}

AgentExportWriteId makeAgentExportWriteId(
    const std::string& manifest_export_id,
    size_t file_count) {
    return AgentExportWriteId("agent_export_write_" + manifest_export_id + "_" + std::to_string(file_count));
}

AgentExportVerifyId makeAgentExportVerifyId(
    const std::string& write_id) {
    return AgentExportVerifyId("agent_export_verify_" + write_id);
}

// --- Enum toString / fromString ---

std::string toString(AgentExportFileKind kind) {
    switch (kind) {
        case AgentExportFileKind::Unknown: return "Unknown";
        case AgentExportFileKind::Manifest: return "Manifest";
        case AgentExportFileKind::Chunk: return "Chunk";
        case AgentExportFileKind::Diagnostics: return "Diagnostics";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentExportFileKind> agentExportFileKindFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentExportFileKind>::ok(AgentExportFileKind::Unknown);
    if (str == "Manifest") return pathfinder::foundation::Result<AgentExportFileKind>::ok(AgentExportFileKind::Manifest);
    if (str == "Chunk") return pathfinder::foundation::Result<AgentExportFileKind>::ok(AgentExportFileKind::Chunk);
    if (str == "Diagnostics") return pathfinder::foundation::Result<AgentExportFileKind>::ok(AgentExportFileKind::Diagnostics);
    return pathfinder::foundation::Result<AgentExportFileKind>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "export_file_kind_from_string_failed"));
}

std::string toString(AgentExportFileExtension ext) {
    switch (ext) {
        case AgentExportFileExtension::Unknown: return "Unknown";
        case AgentExportFileExtension::Txt: return "Txt";
        case AgentExportFileExtension::Md: return "Md";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentExportFileExtension> agentExportFileExtensionFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentExportFileExtension>::ok(AgentExportFileExtension::Unknown);
    if (str == "Txt") return pathfinder::foundation::Result<AgentExportFileExtension>::ok(AgentExportFileExtension::Txt);
    if (str == "Md") return pathfinder::foundation::Result<AgentExportFileExtension>::ok(AgentExportFileExtension::Md);
    return pathfinder::foundation::Result<AgentExportFileExtension>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "export_file_extension_from_string_failed"));
}

std::string toString(AgentExportWriteStatus status) {
    switch (status) {
        case AgentExportWriteStatus::Unknown: return "Unknown";
        case AgentExportWriteStatus::Planned: return "Planned";
        case AgentExportWriteStatus::Written: return "Written";
        case AgentExportWriteStatus::Failed: return "Failed";
        case AgentExportWriteStatus::Skipped: return "Skipped";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentExportWriteStatus> agentExportWriteStatusFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentExportWriteStatus>::ok(AgentExportWriteStatus::Unknown);
    if (str == "Planned") return pathfinder::foundation::Result<AgentExportWriteStatus>::ok(AgentExportWriteStatus::Planned);
    if (str == "Written") return pathfinder::foundation::Result<AgentExportWriteStatus>::ok(AgentExportWriteStatus::Written);
    if (str == "Failed") return pathfinder::foundation::Result<AgentExportWriteStatus>::ok(AgentExportWriteStatus::Failed);
    if (str == "Skipped") return pathfinder::foundation::Result<AgentExportWriteStatus>::ok(AgentExportWriteStatus::Skipped);
    return pathfinder::foundation::Result<AgentExportWriteStatus>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "export_write_status_from_string_failed"));
}

std::string toString(AgentExportVerificationStatus status) {
    switch (status) {
        case AgentExportVerificationStatus::Unknown: return "Unknown";
        case AgentExportVerificationStatus::Passed: return "Passed";
        case AgentExportVerificationStatus::Failed: return "Failed";
        case AgentExportVerificationStatus::Warning: return "Warning";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentExportVerificationStatus> agentExportVerificationStatusFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentExportVerificationStatus>::ok(AgentExportVerificationStatus::Unknown);
    if (str == "Passed") return pathfinder::foundation::Result<AgentExportVerificationStatus>::ok(AgentExportVerificationStatus::Passed);
    if (str == "Failed") return pathfinder::foundation::Result<AgentExportVerificationStatus>::ok(AgentExportVerificationStatus::Failed);
    if (str == "Warning") return pathfinder::foundation::Result<AgentExportVerificationStatus>::ok(AgentExportVerificationStatus::Warning);
    return pathfinder::foundation::Result<AgentExportVerificationStatus>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "export_verification_status_from_string_failed"));
}

// --- AgentExportPathPolicy::validateBasic ---

pathfinder::foundation::Result<void> AgentExportPathPolicy::validateBasic() const {
    if (root_dir.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "path_policy_empty_root_dir"));
    }
    if (root_dir.find("..") != std::string::npos) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "path_policy_root_dir_traversal"));
    }
    if (max_file_count == 0 || max_file_count > 1024) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "path_policy_max_file_count_out_of_range"));
    }
    if (max_file_bytes == 0) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "path_policy_max_file_bytes_zero"));
    }
    if (max_total_bytes < max_file_bytes) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "path_policy_max_total_bytes_less_than_max_file_bytes"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportFilePlan::validateBasic ---

pathfinder::foundation::Result<void> AgentExportFilePlan::validateBasic() const {
    if (file_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "file_plan_empty_file_id"));
    }
    if (kind == AgentExportFileKind::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "file_plan_unknown_kind"));
    }
    if (extension == AgentExportFileExtension::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "file_plan_unknown_extension"));
    }
    if (relative_path.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_empty_relative_path"));
    }
    if (!relative_path.empty() && relative_path[0] == '/') {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_absolute_relative_path"));
    }
    if (relative_path.find("..") != std::string::npos) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_relative_path_traversal"));
    }
    if (relative_path.find('\\') != std::string::npos) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_relative_path_backslash"));
    }
    if (!isValidRelativePathComponent(relative_path)) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_relative_path_invalid_chars"));
    }
    if (kind == AgentExportFileKind::Manifest && content.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_manifest_empty_content"));
    }
    if (byte_size != content.size()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_plan_byte_size_mismatch"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportWritePlan::validateBasic ---

pathfinder::foundation::Result<void> AgentExportWritePlan::validateBasic() const {
    if (write_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "write_plan_empty_write_id"));
    }
    if (export_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_empty_export_id"));
    }
    if (files.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_empty_files"));
    }
    if (files.size() > path_policy.max_file_count) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "write_plan_exceeds_max_file_count"));
    }

    size_t computed_total = 0;
    bool has_manifest = false;
    std::set<std::string> seen_paths;

    for (const auto& f : files) {
        auto v = f.validateBasic();
        if (v.is_error()) return v;

        if (f.byte_size > path_policy.max_file_bytes) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                    "write_plan_file_exceeds_max_file_bytes"));
        }

        computed_total += f.byte_size;

        if (f.kind == AgentExportFileKind::Manifest) {
            has_manifest = true;
        }

        if (!seen_paths.insert(f.relative_path).second) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "write_plan_duplicate_relative_path"));
        }
    }

    if (!has_manifest) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_missing_manifest"));
    }

    if (total_bytes != computed_total) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_total_bytes_mismatch"));
    }

    if (total_bytes > path_policy.max_total_bytes) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "write_plan_exceeds_max_total_bytes"));
    }

    return path_policy.validateBasic();
}

// --- AgentExportFileWriteResult::validateBasic ---

pathfinder::foundation::Result<void> AgentExportFileWriteResult::validateBasic() const {
    if (file_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "file_write_result_empty_file_id"));
    }
    if (kind == AgentExportFileKind::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "file_write_result_unknown_kind"));
    }
    if (status == AgentExportWriteStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "file_write_result_unknown_status"));
    }
    if (relative_path.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_write_result_empty_relative_path"));
    }
    if (status == AgentExportWriteStatus::Written && !error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_write_result_written_with_errors"));
    }
    if (status == AgentExportWriteStatus::Failed && error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "file_write_result_failed_without_errors"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportWriteResult::validateBasic ---

pathfinder::foundation::Result<void> AgentExportWriteResult::validateBasic() const {
    if (write_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "write_result_empty_write_id"));
    }
    if (status == AgentExportWriteStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "write_result_unknown_status"));
    }
    if (root_dir.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_result_empty_root_dir"));
    }

    size_t written_count = 0;
    for (const auto& f : files) {
        auto v = f.validateBasic();
        if (v.is_error()) return v;
        if (f.status == AgentExportWriteStatus::Written) {
            ++written_count;
        }
    }

    if (written_file_count != written_count) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_result_written_count_mismatch"));
    }

    if (status == AgentExportWriteStatus::Written && !error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_result_written_with_errors"));
    }
    if (status == AgentExportWriteStatus::Failed && error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_result_failed_without_errors"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportVerificationReport::validateBasic ---

pathfinder::foundation::Result<void> AgentExportVerificationReport::validateBasic() const {
    if (verify_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "verify_report_empty_verify_id"));
    }
    if (write_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_missing,
                "verify_report_empty_write_id"));
    }
    if (status == AgentExportVerificationStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "verify_report_unknown_status"));
    }
    if (status == AgentExportVerificationStatus::Passed) {
        if (expected_file_count != actual_file_count) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "verify_report_passed_file_count_mismatch"));
        }
        if (expected_total_bytes != actual_total_bytes) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "verify_report_passed_total_bytes_mismatch"));
        }
        if (!error_keys.empty()) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "verify_report_passed_with_errors"));
        }
    }
    if (status == AgentExportVerificationStatus::Failed && error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "verify_report_failed_without_errors"));
    }

    // Check for duplicate paths
    std::set<std::string> seen(checked_relative_paths.begin(), checked_relative_paths.end());
    if (seen.size() != checked_relative_paths.size()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "verify_report_duplicate_checked_paths"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportWritePlanRequest::validateBasic ---

pathfinder::foundation::Result<void> AgentExportWritePlanRequest::validateBasic() const {
    auto dv = draft.validateBasic();
    if (dv.is_error()) return dv;

    auto pv = path_policy.validateBasic();
    if (pv.is_error()) return pv;

    if (base_name.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_request_empty_base_name"));
    }
    if (!isValidBaseName(base_name)) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "write_plan_request_invalid_base_name"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- Helper: format to extension ---

static AgentExportFileExtension extensionForFormat(AgentExportFormat format) {
    switch (format) {
        case AgentExportFormat::PlainText:
        case AgentExportFormat::ProtocolText:
            return AgentExportFileExtension::Txt;
        case AgentExportFormat::MarkdownLike:
            return AgentExportFileExtension::Md;
        default:
            return AgentExportFileExtension::Unknown;
    }
}

static std::string extensionString(AgentExportFileExtension ext) {
    switch (ext) {
        case AgentExportFileExtension::Txt: return ".txt";
        case AgentExportFileExtension::Md: return ".md";
        default: return "";
    }
}

static std::string kindString(AgentExportFileKind kind) {
    switch (kind) {
        case AgentExportFileKind::Manifest: return "manifest";
        case AgentExportFileKind::Chunk: return "chunk";
        case AgentExportFileKind::Diagnostics: return "diagnostics";
        default: return "unknown";
    }
}

// --- AgentExportWritePlanner::buildPlan ---

pathfinder::foundation::Result<AgentExportWritePlan> AgentExportWritePlanner::buildPlan(
    const AgentExportWritePlanRequest& request) const {
    auto rv = request.validateBasic();
    if (rv.is_error()) return pathfinder::foundation::Result<AgentExportWritePlan>::fail(rv.errors());

    auto ext = extensionForFormat(request.draft.manifest.format);
    if (ext == AgentExportFileExtension::Unknown) {
        return pathfinder::foundation::Result<AgentExportWritePlan>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "write_plan_unknown_format"));
    }

    std::string ext_str = extensionString(ext);
    std::vector<AgentExportFilePlan> file_plans;
    size_t computed_total = 0;

    // Manifest file
    if (request.include_manifest_file) {
        std::string manifest_content = "manifest_id=" + request.draft.manifest.manifest_id
            + "\nformat=" + toString(request.draft.manifest.format)
            + "\nchunk_count=" + std::to_string(request.draft.manifest.chunk_count)
            + "\ntotal_item_count=" + std::to_string(request.draft.manifest.total_item_count);

        AgentExportFilePlan mp;
        mp.file_id = makeAgentExportFileId(request.draft.manifest.manifest_id, "manifest", 0);
        mp.kind = AgentExportFileKind::Manifest;
        mp.extension = ext;
        mp.relative_path = request.base_name + "_manifest" + ext_str;
        mp.content = manifest_content;
        mp.byte_size = manifest_content.size();

        file_plans.push_back(mp);
        computed_total += mp.byte_size;
    }

    // Chunk files
    for (size_t i = 0; i < request.draft.chunks.size(); ++i) {
        const auto& chunk = request.draft.chunks[i];

        AgentExportFilePlan cp;
        cp.file_id = makeAgentExportFileId(request.draft.manifest.manifest_id, "chunk", i);
        cp.kind = AgentExportFileKind::Chunk;
        cp.extension = ext;
        cp.relative_path = request.base_name + "_chunk_" + std::to_string(i) + ext_str;
        cp.content = chunk.payload;
        cp.byte_size = chunk.payload.size();

        file_plans.push_back(cp);
        computed_total += cp.byte_size;
    }

    AgentExportWritePlan plan;
    plan.write_id = makeAgentExportWriteId(request.draft.manifest.manifest_id, file_plans.size());
    plan.export_id = request.draft.manifest.manifest_id;
    plan.path_policy = request.path_policy;
    plan.files = std::move(file_plans);
    plan.total_bytes = computed_total;

    auto pv = plan.validateBasic();
    if (pv.is_error()) return pathfinder::foundation::Result<AgentExportWritePlan>::fail(pv.errors());

    return pathfinder::foundation::Result<AgentExportWritePlan>::ok(std::move(plan));
}

// --- AgentLocalExportService::write ---

pathfinder::foundation::Result<AgentExportWriteResult> AgentLocalExportService::write(
    const AgentExportWritePlan& plan) const {
    auto pv = plan.validateBasic();
    if (pv.is_error()) return pathfinder::foundation::Result<AgentExportWriteResult>::fail(pv.errors());

    AgentExportWriteResult result;
    result.write_id = plan.write_id;
    result.root_dir = plan.path_policy.root_dir;
    result.planned_file_count = plan.files.size();
    result.total_bytes = plan.total_bytes;

    // Create root_dir if needed
    std::filesystem::path root(plan.path_policy.root_dir);
    if (!std::filesystem::exists(root)) {
        if (!plan.path_policy.allow_create_directories) {
            result.status = AgentExportWriteStatus::Failed;
            result.error_keys.push_back("export_root_dir_not_found");
            for (const auto& f : plan.files) {
                AgentExportFileWriteResult fr;
                fr.file_id = f.file_id;
                fr.kind = f.kind;
                fr.status = AgentExportWriteStatus::Failed;
                fr.relative_path = f.relative_path;
                fr.error_keys.push_back("export_root_dir_not_found");
                result.files.push_back(std::move(fr));
            }
            return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
        }
        std::error_code ec;
        std::filesystem::create_directories(root, ec);
        if (ec) {
            result.status = AgentExportWriteStatus::Failed;
            result.error_keys.push_back("export_create_directory_failed");
            return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
        }
    }

    // Write each file
    size_t written_count = 0;
    for (const auto& f : plan.files) {
        AgentExportFileWriteResult fr;
        fr.file_id = f.file_id;
        fr.kind = f.kind;
        fr.relative_path = f.relative_path;
        fr.byte_size = f.byte_size;

        std::filesystem::path target = root / f.relative_path;

        // Check overwrite policy
        if (std::filesystem::exists(target) && !plan.path_policy.allow_overwrite) {
            fr.status = AgentExportWriteStatus::Failed;
            fr.error_keys.push_back("export_file_already_exists");
            result.files.push_back(fr);
            result.status = AgentExportWriteStatus::Failed;
            result.error_keys.push_back("export_file_already_exists");
            return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
        }

        // Create parent directories if needed
        std::filesystem::path parent = target.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                fr.status = AgentExportWriteStatus::Failed;
                fr.error_keys.push_back("export_create_parent_directory_failed");
                result.files.push_back(fr);
                result.status = AgentExportWriteStatus::Failed;
                result.error_keys.push_back("export_create_parent_directory_failed");
                return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
            }
        }

        // Write file
        std::ofstream ofs(target);
        if (!ofs.is_open()) {
            fr.status = AgentExportWriteStatus::Failed;
            fr.error_keys.push_back("export_file_open_failed");
            result.files.push_back(fr);
            result.status = AgentExportWriteStatus::Failed;
            result.error_keys.push_back("export_file_open_failed");
            return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
        }

        ofs.write(f.content.data(), static_cast<std::streamsize>(f.content.size()));
        if (ofs.fail()) {
            fr.status = AgentExportWriteStatus::Failed;
            fr.error_keys.push_back("export_file_write_failed");
            result.files.push_back(fr);
            result.status = AgentExportWriteStatus::Failed;
            result.error_keys.push_back("export_file_write_failed");
            return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
        }

        ofs.close();
        fr.status = AgentExportWriteStatus::Written;
        ++written_count;
        result.files.push_back(fr);
    }

    result.written_file_count = written_count;
    result.status = AgentExportWriteStatus::Written;

    return pathfinder::foundation::Result<AgentExportWriteResult>::ok(std::move(result));
}

// --- AgentExportVerifyRequest::validateBasic ---

pathfinder::foundation::Result<void> AgentExportVerifyRequest::validateBasic() const {
    auto pv = plan.validateBasic();
    if (pv.is_error()) return pv;

    auto wv = write_result.validateBasic();
    if (wv.is_error()) return wv;

    if (plan.write_id != write_result.write_id) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "verify_request_write_id_mismatch"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- Forbidden terms ---

static const std::vector<std::string>& forbiddenTerms() {
    static const std::vector<std::string> terms = {
        "GameState",
        "ObjectDefinition",
        "edible_profile",
        "hunger_delta",
        "health_delta",
        "effect_kind",
        "reward_value",
        "is_done",
        "done ="
    };
    return terms;
}

static bool containsForbiddenTerm(const std::string& content, std::string& found_term) {
    for (const auto& term : forbiddenTerms()) {
        if (content.find(term) != std::string::npos) {
            found_term = term;
            return true;
        }
    }
    return false;
}

// --- AgentExportVerifier::verify ---

pathfinder::foundation::Result<AgentExportVerificationReport> AgentExportVerifier::verify(
    const AgentExportVerifyRequest& request) const {
    auto rv = request.validateBasic();
    if (rv.is_error()) return pathfinder::foundation::Result<AgentExportVerificationReport>::fail(rv.errors());

    AgentExportVerificationReport report;
    report.verify_id = makeAgentExportVerifyId(request.write_result.write_id.value());
    report.write_id = request.write_result.write_id;
    report.expected_file_count = request.plan.files.size();
    report.expected_total_bytes = request.plan.total_bytes;

    std::filesystem::path root(request.plan.path_policy.root_dir);
    size_t actual_count = 0;
    size_t actual_bytes = 0;

    for (const auto& f : request.plan.files) {
        std::filesystem::path target = root / f.relative_path;
        report.checked_relative_paths.push_back(f.relative_path);

        if (!std::filesystem::exists(target)) {
            report.error_keys.push_back("verify_file_missing_" + f.relative_path);
            continue;
        }

        auto file_size = std::filesystem::file_size(target);
        if (file_size != f.byte_size) {
            report.error_keys.push_back("verify_file_size_mismatch_" + f.relative_path);
            continue;
        }

        ++actual_count;
        actual_bytes += file_size;

        // Scan content for forbidden terms
        if (request.scan_content_for_forbidden_terms) {
            std::ifstream ifs(target);
            if (ifs.is_open()) {
                std::string content((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());
                std::string found_term;
                if (containsForbiddenTerm(content, found_term)) {
                    report.error_keys.push_back("verify_forbidden_term_" + found_term + "_in_" + f.relative_path);
                }
            }
        }
    }

    report.actual_file_count = actual_count;
    report.actual_total_bytes = actual_bytes;

    if (report.error_keys.empty()) {
        report.status = AgentExportVerificationStatus::Passed;
    } else {
        report.status = AgentExportVerificationStatus::Failed;
    }

    return pathfinder::foundation::Result<AgentExportVerificationReport>::ok(std::move(report));
}

} // namespace pathfinder::agent
