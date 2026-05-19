#include "pathfinder/save/save_package.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <utility>

namespace pathfinder::save {

using pathfinder::foundation::ConfigVersionId;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::HashValue;
using pathfinder::foundation::Result;
using pathfinder::foundation::StateVersion;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

static Result<void> fail(ErrorCode code, const std::string& message) {
    return Result<void>::fail(makeError(code, message));
}

static Result<void> validateIdValue(const std::string& value, const std::string& field) {
    if (value.empty()) return fail(ErrorCode::id_missing, field + " missing");
    if (!isValidIdString(value)) return fail(ErrorCode::id_invalid_format, field + " invalid");
    if (containsSaveForbiddenKey(value)) return fail(ErrorCode::validation_failed, field + " forbidden");
    return Result<void>::ok();
}

static Result<void> validateSafeKey(const std::string& value, const std::string& field, bool required = true) {
    if (value.empty()) {
        if (required) return fail(ErrorCode::validation_failed, field + " empty");
        return Result<void>::ok();
    }
    if (containsSaveForbiddenKey(value)) return fail(ErrorCode::validation_failed, field + " forbidden");
    return Result<void>::ok();
}

static Result<void> validateSafeKeys(const std::vector<std::string>& keys, const std::string& field) {
    if (containsSaveForbiddenKey(keys)) return fail(ErrorCode::validation_failed, field + " forbidden");
    return Result<void>::ok();
}

static HashValue hashText(const std::string& text) {
    return HashValue::fromString(text.empty() ? "empty" : text);
}

static std::string versionText(const StateVersion& version) {
    return std::to_string(version.value());
}

static HashValue combine(HashValue a, HashValue b) {
    return HashValue::combineHash(a, b);
}

static HashValue hashVector(const std::vector<std::string>& values, const std::string& prefix) {
    HashValue out = hashText(prefix + ":" + std::to_string(values.size()));
    for (const auto& value : values) out = combine(out, hashText(value));
    return out;
}

static HashValue hashEventIds(const std::vector<pathfinder::foundation::EventId>& ids, const std::string& prefix) {
    HashValue out = hashText(prefix + ":" + std::to_string(ids.size()));
    for (const auto& id : ids) out = combine(out, hashText(id.value()));
    return out;
}

static HashValue hashHeader(const SaveHeader& header, bool include_package_hash) {
    HashValue out = hashText("header");
    out = combine(out, hashText(header.save_id.value()));
    out = combine(out, hashText(header.save_format_version));
    out = combine(out, hashText(header.game_build_version));
    out = combine(out, hashText(header.game_id.value()));
    out = combine(out, hashText(header.player_id.value()));
    out = combine(out, hashText(header.config_version.value()));
    out = combine(out, hashText(std::to_string(header.created_tick.value())));
    out = combine(out, hashText(versionText(header.base_state_version)));
    out = combine(out, hashText(versionText(header.latest_state_version)));
    if (include_package_hash) out = combine(out, HashValue(header.package_hash.value()));
    return out;
}

static HashValue hashIndex(const SaveIndex& index) {
    HashValue out = hashText("index:" + std::to_string(index.entries.size()));
    for (const auto& entry : index.entries) {
        HashValue entry_hash = hashText(toString(entry.kind));
        entry_hash = combine(entry_hash, hashText(entry.section_key));
        entry_hash = combine(entry_hash, entry.section_hash);
        entry_hash = combine(entry_hash, hashText(std::to_string(entry.record_count)));
        entry_hash = combine(entry_hash, hashText(versionText(entry.from_state_version)));
        entry_hash = combine(entry_hash, hashText(versionText(entry.to_state_version)));
        out = combine(out, entry_hash);
    }
    return out;
}

static HashValue hashSnapshots(const StateSnapshotSection& section) {
    HashValue out = hashText("snapshots:" + std::to_string(section.snapshots.size()));
    for (const auto& snapshot : section.snapshots) {
        HashValue item = hashText(snapshot.snapshot_id.value());
        item = combine(item, hashText(snapshot.snapshot.metadata.state_id.value()));
        item = combine(item, hashText(versionText(snapshot.snapshot.metadata.state_version)));
        item = combine(item, hashText(std::to_string(snapshot.snapshot.metadata.current_tick.value())));
        item = combine(item, hashText(snapshot.snapshot.metadata.config_version.value()));
        item = combine(item, snapshot.snapshot.metadata.state_hash);
        item = combine(item, snapshot.snapshot_hash);
        item = combine(item, hashVector(snapshot.reason_keys, "snapshot_reasons"));
        out = combine(out, item);
    }
    return out;
}

static HashValue hashEvents(const EventLogSection& section) {
    HashValue out = hashText("events:" + std::to_string(section.events.size()));
    for (const auto& event : section.events) {
        HashValue item = hashText(event.event_id.value());
        item = combine(item, hashText(pathfinder::event::toString(event.event_type)));
        item = combine(item, hashText(event.command_id.has_value() ? event.command_id->value() : "none"));
        item = combine(item, hashText(std::to_string(event.tick.value())));
        item = combine(item, hashText(versionText(event.state_version)));
        item = combine(item, hashText(event.payload.payload_type));
        item = combine(item, hashText(event.payload.message_key));
        out = combine(out, item);
    }
    return out;
}

static HashValue hashCommandLog(const CommandLogSection& section) {
    HashValue out = hashText("commands:" + std::to_string(section.command_log.records().size()));
    for (const auto& record : section.command_log.records()) {
        HashValue item = hashText(record.record_id.value());
        item = combine(item, hashText(record.command_id.value()));
        item = combine(item, hashText(versionText(record.state_version_before)));
        item = combine(item, hashText(versionText(record.state_version_after)));
        item = combine(item, HashValue(record.random_seed.value()));
        item = combine(item, record.input_hash);
        item = combine(item, record.output_hash);
        item = combine(item, hashText(pathfinder::pipeline::toString(record.pipeline_status)));
        item = combine(item, hashText(pathfinder::replay::toString(record.status)));
        out = combine(out, item);
    }
    return out;
}

static HashValue hashRandomLog(const RandomLogSection& section) {
    HashValue out = hashText("random:" + std::to_string(section.random_log.entries().size()));
    for (const auto& entry : section.random_log.entries()) {
        HashValue item = hashText(entry.entry_id.value());
        item = combine(item, hashText(entry.command_id.value()));
        item = combine(item, HashValue(entry.seed.value()));
        item = combine(item, hashText(std::to_string(entry.draw_index)));
        item = combine(item, hashText(std::to_string(entry.result_value)));
        item = combine(item, hashText(entry.reason));
        out = combine(out, item);
    }
    return out;
}

static HashValue hashTrace(const PipelineTraceSection& section) {
    HashValue out = hashText("trace:" + std::to_string(section.traces.size()));
    for (const auto& trace : section.traces) {
        HashValue item = hashText(trace.trace_id.value());
        item = combine(item, hashText(trace.command_id.has_value() ? trace.command_id->value() : "none"));
        item = combine(item, hashText(versionText(trace.state_version)));
        item = combine(item, hashVector(trace.pipeline_stage_keys, "stages"));
        item = combine(item, hashEventIds(trace.event_ids, "trace_events"));
        item = combine(item, hashVector(trace.condition_trace_keys, "conditions"));
        item = combine(item, hashVector(trace.reason_keys, "trace_reasons"));
        out = combine(out, item);
    }
    return out;
}

static HashValue hashConfig(const ConfigVersionManifest& manifest) {
    HashValue out = hashText("config");
    out = combine(out, hashText(manifest.active_config_version.value()));
    out = combine(out, manifest.config_content_hash);
    out = combine(out, hashText(manifest.compatibility_key));
    for (const auto& version : manifest.required_config_versions) out = combine(out, hashText(version.value()));
    return out;
}

static HashValue hashJournal(const std::optional<CrashRecoveryJournal>& journal) {
    if (!journal.has_value()) return hashText("journal:none");
    HashValue out = hashText("journal");
    out = combine(out, hashText(journal->journal_id.value()));
    out = combine(out, hashText(journal->open_transaction_key));
    out = combine(out, hashText(journal->command_id.has_value() ? journal->command_id->value() : "none"));
    out = combine(out, hashText(std::to_string(journal->started_tick.value())));
    out = combine(out, hashText(versionText(journal->state_version_before)));
    out = combine(out, hashText(journal->recovery_action_key));
    out = combine(out, hashVector(journal->reason_keys, "journal_reasons"));
    return out;
}

static SaveIndexEntry makeEntry(SaveSectionKind kind, std::string key, HashValue hash, size_t count, StateVersion from, StateVersion to) {
    SaveIndexEntry entry;
    entry.kind = kind;
    entry.section_key = std::move(key);
    entry.section_hash = hash;
    entry.record_count = count;
    entry.from_state_version = from;
    entry.to_state_version = to;
    return entry;
}

static StateVersion minVersion(StateVersion a, StateVersion b) { return a <= b ? a : b; }
static StateVersion maxVersion(StateVersion a, StateVersion b) { return a >= b ? a : b; }

static std::pair<StateVersion, StateVersion> snapshotRange(const StateSnapshotSection& section) {
    StateVersion from = section.snapshots.front().snapshot.metadata.state_version;
    StateVersion to = from;
    for (const auto& snapshot : section.snapshots) {
        from = minVersion(from, snapshot.snapshot.metadata.state_version);
        to = maxVersion(to, snapshot.snapshot.metadata.state_version);
    }
    return {from, to};
}

static std::pair<StateVersion, StateVersion> eventRange(const EventLogSection& section, StateVersion fallback) {
    if (section.events.empty()) return {fallback, fallback};
    StateVersion from = section.events.front().state_version;
    StateVersion to = from;
    for (const auto& event : section.events) {
        from = minVersion(from, event.state_version);
        to = maxVersion(to, event.state_version);
    }
    return {from, to};
}

static std::pair<StateVersion, StateVersion> commandRange(const CommandLogSection& section, StateVersion fallback) {
    if (section.command_log.records().empty()) return {fallback, fallback};
    StateVersion from = section.command_log.records().front().state_version_before;
    StateVersion to = section.command_log.records().front().state_version_after;
    for (const auto& record : section.command_log.records()) {
        from = minVersion(from, record.state_version_before);
        to = maxVersion(to, record.state_version_after);
    }
    return {from, to};
}

static std::pair<StateVersion, StateVersion> traceRange(const PipelineTraceSection& section, StateVersion fallback) {
    if (section.traces.empty()) return {fallback, fallback};
    StateVersion from = section.traces.front().state_version;
    StateVersion to = from;
    for (const auto& trace : section.traces) {
        from = minVersion(from, trace.state_version);
        to = maxVersion(to, trace.state_version);
    }
    return {from, to};
}

std::string toString(SaveSectionKind kind) {
    switch (kind) {
        case SaveSectionKind::Unknown: return "unknown";
        case SaveSectionKind::Header: return "header";
        case SaveSectionKind::StateSnapshot: return "state_snapshot";
        case SaveSectionKind::EventLog: return "event_log";
        case SaveSectionKind::CommandLog: return "command_log";
        case SaveSectionKind::RandomLog: return "random_log";
        case SaveSectionKind::PipelineTrace: return "pipeline_trace";
        case SaveSectionKind::ConfigManifest: return "config_manifest";
        case SaveSectionKind::IntegrityManifest: return "integrity_manifest";
        case SaveSectionKind::CrashRecoveryJournal: return "crash_recovery_journal";
        case SaveSectionKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(RestorePlanStepKind kind) {
    switch (kind) {
        case RestorePlanStepKind::Unknown: return "unknown";
        case RestorePlanStepKind::VerifyIntegrity: return "verify_integrity";
        case RestorePlanStepKind::LoadConfigManifest: return "load_config_manifest";
        case RestorePlanStepKind::LoadSnapshot: return "load_snapshot";
        case RestorePlanStepKind::ApplyEventLog: return "apply_event_log";
        case RestorePlanStepKind::AttachReplayLogs: return "attach_replay_logs";
        case RestorePlanStepKind::AttachTraceLog: return "attach_trace_log";
        case RestorePlanStepKind::Ready: return "ready";
        case RestorePlanStepKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<SaveSectionKind> saveSectionKindFromString(const std::string& value) {
    if (value == "unknown") return Result<SaveSectionKind>::ok(SaveSectionKind::Unknown);
    if (value == "header") return Result<SaveSectionKind>::ok(SaveSectionKind::Header);
    if (value == "state_snapshot") return Result<SaveSectionKind>::ok(SaveSectionKind::StateSnapshot);
    if (value == "event_log") return Result<SaveSectionKind>::ok(SaveSectionKind::EventLog);
    if (value == "command_log") return Result<SaveSectionKind>::ok(SaveSectionKind::CommandLog);
    if (value == "random_log") return Result<SaveSectionKind>::ok(SaveSectionKind::RandomLog);
    if (value == "pipeline_trace") return Result<SaveSectionKind>::ok(SaveSectionKind::PipelineTrace);
    if (value == "config_manifest") return Result<SaveSectionKind>::ok(SaveSectionKind::ConfigManifest);
    if (value == "integrity_manifest") return Result<SaveSectionKind>::ok(SaveSectionKind::IntegrityManifest);
    if (value == "crash_recovery_journal") return Result<SaveSectionKind>::ok(SaveSectionKind::CrashRecoveryJournal);
    if (value == "test_only") return Result<SaveSectionKind>::ok(SaveSectionKind::TestOnly);
    return Result<SaveSectionKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid SaveSectionKind: " + value));
}

Result<RestorePlanStepKind> restorePlanStepKindFromString(const std::string& value) {
    if (value == "unknown") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::Unknown);
    if (value == "verify_integrity") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::VerifyIntegrity);
    if (value == "load_config_manifest") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::LoadConfigManifest);
    if (value == "load_snapshot") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::LoadSnapshot);
    if (value == "apply_event_log") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::ApplyEventLog);
    if (value == "attach_replay_logs") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::AttachReplayLogs);
    if (value == "attach_trace_log") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::AttachTraceLog);
    if (value == "ready") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::Ready);
    if (value == "test_only") return Result<RestorePlanStepKind>::ok(RestorePlanStepKind::TestOnly);
    return Result<RestorePlanStepKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid RestorePlanStepKind: " + value));
}

bool containsSaveForbiddenKey(const std::string& key) {
    const auto lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "true_trait", "real_effect", "raw_state", "gamestate",
        "actual_hp", "true_hp", "hp_delta", "death", "kill", "corpse",
        "loot", "drop", "random_damage", "frontend_unlock", "direct_state"
    };
    return std::any_of(forbidden.begin(), forbidden.end(), [&](const auto& token) { return lower.find(token) != std::string::npos; });
}

bool containsSaveForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) { return containsSaveForbiddenKey(key); });
}

Result<void> SaveHeader::validateBasic(bool require_package_hash) const {
    auto save = validateIdValue(save_id.value(), "SaveHeader save_id"); if (save.is_error()) return save;
    if (save_format_version != "save_package.v1") return fail(ErrorCode::validation_failed, "SaveHeader format unsupported");
    auto build = validateSafeKey(game_build_version, "SaveHeader game_build_version"); if (build.is_error()) return build;
    auto game = validateIdValue(game_id.value(), "SaveHeader game_id"); if (game.is_error()) return game;
    auto player = validateIdValue(player_id.value(), "SaveHeader player_id"); if (player.is_error()) return player;
    auto config = validateIdValue(config_version.value(), "SaveHeader config_version"); if (config.is_error()) return config;
    if (latest_state_version < base_state_version) return fail(ErrorCode::validation_failed, "SaveHeader latest before base");
    if (require_package_hash && package_hash.empty()) return fail(ErrorCode::validation_failed, "SaveHeader package_hash empty");
    return Result<void>::ok();
}

Result<void> SaveIndexEntry::validateBasic() const {
    if (kind == SaveSectionKind::Unknown || kind == SaveSectionKind::TestOnly) return fail(ErrorCode::validation_enum_unknown, "SaveIndexEntry kind invalid");
    auto key = validateSafeKey(section_key, "SaveIndexEntry section_key"); if (key.is_error()) return key;
    if (section_hash.empty()) return fail(ErrorCode::validation_failed, "SaveIndexEntry section_hash empty");
    if (to_state_version < from_state_version) return fail(ErrorCode::validation_failed, "SaveIndexEntry version range invalid");
    return Result<void>::ok();
}

const SaveIndexEntry* SaveIndex::find(SaveSectionKind kind) const {
    for (const auto& entry : entries) if (entry.kind == kind) return &entry;
    return nullptr;
}

Result<void> SaveIndex::validateBasic() const {
    if (entries.empty()) return fail(ErrorCode::validation_failed, "SaveIndex entries empty");
    std::set<std::string> seen;
    for (const auto& entry : entries) {
        auto valid = entry.validateBasic(); if (valid.is_error()) return valid;
        if (!seen.insert(toString(entry.kind)).second) return fail(ErrorCode::validation_failed, "SaveIndex duplicate section kind");
    }
    return Result<void>::ok();
}

Result<void> StateSnapshotRecord::validateBasic() const {
    auto id = validateIdValue(snapshot_id.value(), "StateSnapshotRecord snapshot_id"); if (id.is_error()) return id;
    auto snapshot_valid = snapshot.validateBasic(); if (snapshot_valid.is_error()) return snapshot_valid;
    if (snapshot_hash.empty()) return fail(ErrorCode::validation_failed, "StateSnapshotRecord snapshot_hash empty");
    return validateSafeKeys(reason_keys, "StateSnapshotRecord reason_keys");
}

const StateSnapshotRecord* StateSnapshotSection::latestAtOrBefore(StateVersion target_version) const {
    const StateSnapshotRecord* selected = nullptr;
    for (const auto& snapshot : snapshots) {
        const auto version = snapshot.snapshot.metadata.state_version;
        if (version <= target_version && (!selected || selected->snapshot.metadata.state_version < version)) selected = &snapshot;
    }
    return selected;
}

Result<void> StateSnapshotSection::validateBasic() const {
    if (snapshots.empty()) return fail(ErrorCode::validation_failed, "StateSnapshotSection empty");
    std::set<std::string> ids;
    std::set<uint64_t> versions;
    for (const auto& snapshot : snapshots) {
        auto valid = snapshot.validateBasic(); if (valid.is_error()) return valid;
        if (!ids.insert(snapshot.snapshot_id.value()).second) return fail(ErrorCode::validation_failed, "StateSnapshotSection duplicate snapshot_id");
        if (!versions.insert(snapshot.snapshot.metadata.state_version.value()).second) return fail(ErrorCode::validation_failed, "StateSnapshotSection duplicate state_version");
    }
    return Result<void>::ok();
}

Result<void> EventLogSection::validateBasic() const {
    std::set<std::string> ids;
    for (const auto& event : events) {
        auto valid = event.validateBasic(); if (valid.is_error()) return valid;
        if (!ids.insert(event.event_id.value()).second) return fail(ErrorCode::validation_failed, "EventLogSection duplicate event_id");
        if (containsSaveForbiddenKey(event.payload.payload_type) || containsSaveForbiddenKey(event.payload.message_key) || containsSaveForbiddenKey(event.payload.debug_text)) {
            return fail(ErrorCode::validation_failed, "EventLogSection payload forbidden");
        }
    }
    return Result<void>::ok();
}

Result<void> CommandLogSection::validateBasic() const { return command_log.validateBasic(); }
Result<void> RandomLogSection::validateBasic() const { return random_log.validateBasic(); }

Result<void> PipelineTraceRecord::validateBasic() const {
    auto id = validateIdValue(trace_id.value(), "PipelineTraceRecord trace_id"); if (id.is_error()) return id;
    if (command_id.has_value()) { auto cmd = validateIdValue(command_id->value(), "PipelineTraceRecord command_id"); if (cmd.is_error()) return cmd; }
    if (pipeline_stage_keys.empty()) return fail(ErrorCode::validation_failed, "PipelineTraceRecord pipeline_stage_keys empty");
    auto stages = validateSafeKeys(pipeline_stage_keys, "PipelineTraceRecord pipeline_stage_keys"); if (stages.is_error()) return stages;
    auto cond = validateSafeKeys(condition_trace_keys, "PipelineTraceRecord condition_trace_keys"); if (cond.is_error()) return cond;
    auto reasons = validateSafeKeys(reason_keys, "PipelineTraceRecord reason_keys"); if (reasons.is_error()) return reasons;
    for (const auto& event_id : event_ids) {
        auto event = validateIdValue(event_id.value(), "PipelineTraceRecord event_id"); if (event.is_error()) return event;
    }
    return Result<void>::ok();
}

Result<void> PipelineTraceSection::validateBasic() const {
    std::set<std::string> ids;
    for (const auto& trace : traces) {
        auto valid = trace.validateBasic(); if (valid.is_error()) return valid;
        if (!ids.insert(trace.trace_id.value()).second) return fail(ErrorCode::validation_failed, "PipelineTraceSection duplicate trace_id");
    }
    return Result<void>::ok();
}

Result<void> ConfigVersionManifest::validateBasic() const {
    auto active = validateIdValue(active_config_version.value(), "ConfigVersionManifest active_config_version"); if (active.is_error()) return active;
    if (config_content_hash.empty()) return fail(ErrorCode::validation_failed, "ConfigVersionManifest config_content_hash empty");
    auto comp = validateSafeKey(compatibility_key, "ConfigVersionManifest compatibility_key"); if (comp.is_error()) return comp;
    std::set<std::string> seen;
    for (const auto& version : required_config_versions) {
        auto valid = validateIdValue(version.value(), "ConfigVersionManifest required_config_version"); if (valid.is_error()) return valid;
        if (!seen.insert(version.value()).second) return fail(ErrorCode::validation_failed, "ConfigVersionManifest duplicate required_config_version");
    }
    if (seen.find(active_config_version.value()) == seen.end()) return fail(ErrorCode::validation_failed, "ConfigVersionManifest active version missing from required list");
    return Result<void>::ok();
}

Result<void> CrashRecoveryJournal::validateBasic() const {
    auto id = validateIdValue(journal_id.value(), "CrashRecoveryJournal journal_id"); if (id.is_error()) return id;
    auto tx = validateSafeKey(open_transaction_key, "CrashRecoveryJournal open_transaction_key"); if (tx.is_error()) return tx;
    if (command_id.has_value()) { auto cmd = validateIdValue(command_id->value(), "CrashRecoveryJournal command_id"); if (cmd.is_error()) return cmd; }
    auto action = validateSafeKey(recovery_action_key, "CrashRecoveryJournal recovery_action_key"); if (action.is_error()) return action;
    return validateSafeKeys(reason_keys, "CrashRecoveryJournal reason_keys");
}

Result<void> IntegrityManifest::validateBasic() const {
    if (header_hash.empty() || index_hash.empty() || snapshot_hash.empty() || event_log_hash.empty() ||
        command_log_hash.empty() || random_log_hash.empty() || trace_hash.empty() || config_hash.empty() ||
        journal_hash.empty() || package_hash.empty()) {
        return fail(ErrorCode::validation_failed, "IntegrityManifest hash empty");
    }
    return Result<void>::ok();
}

Result<void> SaveGamePackage::validateBasic() const {
    auto header_valid = header.validateBasic(true); if (header_valid.is_error()) return header_valid;
    auto index_valid = index.validateBasic(); if (index_valid.is_error()) return index_valid;
    auto snapshot_valid = snapshot_section.validateBasic(); if (snapshot_valid.is_error()) return snapshot_valid;
    auto event_valid = event_log_section.validateBasic(); if (event_valid.is_error()) return event_valid;
    auto command_valid = command_log_section.validateBasic(); if (command_valid.is_error()) return command_valid;
    auto random_valid = random_log_section.validateBasic(); if (random_valid.is_error()) return random_valid;
    auto trace_valid = trace_section.validateBasic(); if (trace_valid.is_error()) return trace_valid;
    auto config_valid = config_manifest.validateBasic(); if (config_valid.is_error()) return config_valid;
    auto integrity_valid = integrity_manifest.validateBasic(); if (integrity_valid.is_error()) return integrity_valid;
    if (recovery_journal.has_value()) { auto journal_valid = recovery_journal->validateBasic(); if (journal_valid.is_error()) return journal_valid; }
    return Result<void>::ok();
}

Result<void> SavePackageBuilderInput::validateBasic() const {
    auto header_valid = header.validateBasic(false); if (header_valid.is_error()) return header_valid;
    auto snapshot_valid = snapshot_section.validateBasic(); if (snapshot_valid.is_error()) return snapshot_valid;
    auto event_valid = event_log_section.validateBasic(); if (event_valid.is_error()) return event_valid;
    auto command_valid = command_log_section.validateBasic(); if (command_valid.is_error()) return command_valid;
    auto random_valid = random_log_section.validateBasic(); if (random_valid.is_error()) return random_valid;
    auto trace_valid = trace_section.validateBasic(); if (trace_valid.is_error()) return trace_valid;
    auto config_valid = config_manifest.validateBasic(); if (config_valid.is_error()) return config_valid;
    if (recovery_journal.has_value()) { auto journal_valid = recovery_journal->validateBasic(); if (journal_valid.is_error()) return journal_valid; }
    if (header.config_version != config_manifest.active_config_version) return fail(ErrorCode::validation_failed, "SavePackageBuilderInput config mismatch");
    auto [snapshot_from, snapshot_to] = snapshotRange(snapshot_section);
    if (snapshot_from < header.base_state_version || snapshot_to > header.latest_state_version) return fail(ErrorCode::validation_failed, "snapshot range outside header range");
    auto [event_from, event_to] = eventRange(event_log_section, header.base_state_version);
    if (event_from < header.base_state_version || event_to > header.latest_state_version) return fail(ErrorCode::validation_failed, "event range outside header range");
    auto [cmd_from, cmd_to] = commandRange(command_log_section, header.base_state_version);
    if (cmd_from < header.base_state_version || cmd_to > header.latest_state_version) return fail(ErrorCode::validation_failed, "command range outside header range");
    std::set<std::string> command_ids;
    for (const auto& record : command_log_section.command_log.records()) command_ids.insert(record.command_id.value());
    for (const auto& record : command_log_section.command_log.records()) {
        if (random_log_section.random_log.findByCommandId(record.command_id).empty()) {
            return fail(ErrorCode::validation_failed, "command replay record missing matching random log entry");
        }
    }
    for (const auto& entry : random_log_section.random_log.entries()) {
        if (command_ids.find(entry.command_id.value()) == command_ids.end()) {
            return fail(ErrorCode::validation_failed, "random log entry references unknown command_id");
        }
    }
    auto [trace_from, trace_to] = traceRange(trace_section, header.base_state_version);
    if (trace_from < header.base_state_version || trace_to > header.latest_state_version) return fail(ErrorCode::validation_failed, "trace range outside header range");
    return Result<void>::ok();
}

IntegrityManifest calculateIntegrityManifest(const SaveGamePackage& package) {
    IntegrityManifest manifest;
    manifest.header_hash = hashHeader(package.header, false);
    manifest.index_hash = hashIndex(package.index);
    manifest.snapshot_hash = hashSnapshots(package.snapshot_section);
    manifest.event_log_hash = hashEvents(package.event_log_section);
    manifest.command_log_hash = hashCommandLog(package.command_log_section);
    manifest.random_log_hash = hashRandomLog(package.random_log_section);
    manifest.trace_hash = hashTrace(package.trace_section);
    manifest.config_hash = hashConfig(package.config_manifest);
    manifest.journal_hash = hashJournal(package.recovery_journal);
    manifest.sealed = package.integrity_manifest.sealed;
    HashValue package_hash = hashText("package");
    package_hash = combine(package_hash, manifest.header_hash);
    package_hash = combine(package_hash, manifest.index_hash);
    package_hash = combine(package_hash, manifest.snapshot_hash);
    package_hash = combine(package_hash, manifest.event_log_hash);
    package_hash = combine(package_hash, manifest.command_log_hash);
    package_hash = combine(package_hash, manifest.random_log_hash);
    package_hash = combine(package_hash, manifest.trace_hash);
    package_hash = combine(package_hash, manifest.config_hash);
    package_hash = combine(package_hash, manifest.journal_hash);
    manifest.package_hash = package_hash;
    return manifest;
}

HashValue calculateSavePackageHash(const SaveGamePackage& package) {
    return calculateIntegrityManifest(package).package_hash;
}

Result<SaveGamePackage> SavePackageBuilder::build(const SavePackageBuilderInput& input) const {
    auto valid = input.validateBasic();
    if (valid.is_error()) return Result<SaveGamePackage>::fail(valid.errors());

    SaveGamePackage package;
    package.header = input.header;
    package.snapshot_section = input.snapshot_section;
    package.event_log_section = input.event_log_section;
    package.command_log_section = input.command_log_section;
    package.random_log_section = input.random_log_section;
    package.trace_section = input.trace_section;
    package.config_manifest = input.config_manifest;
    package.recovery_journal = input.recovery_journal;

    const auto [snapshot_from, snapshot_to] = snapshotRange(package.snapshot_section);
    const auto [event_from, event_to] = eventRange(package.event_log_section, package.header.base_state_version);
    const auto [cmd_from, cmd_to] = commandRange(package.command_log_section, package.header.base_state_version);
    const auto [trace_from, trace_to] = traceRange(package.trace_section, package.header.base_state_version);

    package.index.entries = {
        makeEntry(SaveSectionKind::Header, "header", hashHeader(package.header, false), 1, package.header.base_state_version, package.header.latest_state_version),
        makeEntry(SaveSectionKind::StateSnapshot, "state_snapshot", hashSnapshots(package.snapshot_section), package.snapshot_section.snapshots.size(), snapshot_from, snapshot_to),
        makeEntry(SaveSectionKind::EventLog, "event_log", hashEvents(package.event_log_section), package.event_log_section.events.size(), event_from, event_to),
        makeEntry(SaveSectionKind::CommandLog, "command_log", hashCommandLog(package.command_log_section), package.command_log_section.command_log.records().size(), cmd_from, cmd_to),
        makeEntry(SaveSectionKind::RandomLog, "random_log", hashRandomLog(package.random_log_section), package.random_log_section.random_log.entries().size(), cmd_from, cmd_to),
        makeEntry(SaveSectionKind::PipelineTrace, "pipeline_trace", hashTrace(package.trace_section), package.trace_section.traces.size(), trace_from, trace_to),
        makeEntry(SaveSectionKind::ConfigManifest, "config_manifest", hashConfig(package.config_manifest), package.config_manifest.required_config_versions.size(), package.header.base_state_version, package.header.latest_state_version)
    };
    if (package.recovery_journal.has_value()) {
        package.index.entries.push_back(makeEntry(SaveSectionKind::CrashRecoveryJournal, "crash_recovery_journal", hashJournal(package.recovery_journal), 1, package.recovery_journal->state_version_before, package.recovery_journal->state_version_before));
    }

    package.integrity_manifest.sealed = true;
    package.integrity_manifest = calculateIntegrityManifest(package);
    package.index.entries.push_back(makeEntry(SaveSectionKind::IntegrityManifest, "integrity_manifest", package.integrity_manifest.package_hash, 1, package.header.base_state_version, package.header.latest_state_version));
    package.integrity_manifest = calculateIntegrityManifest(package);
    package.header.package_hash = package.integrity_manifest.package_hash;

    auto final_valid = SavePackageValidator{}.validate(package);
    if (final_valid.is_error()) return Result<SaveGamePackage>::fail(final_valid.errors());
    return Result<SaveGamePackage>::ok(std::move(package));
}

Result<void> SavePackageValidator::validate(const SaveGamePackage& package) const {
    auto basic = package.validateBasic();
    if (basic.is_error()) return basic;

    const auto expected = calculateIntegrityManifest(package);
    if (package.header.package_hash != expected.package_hash) return fail(ErrorCode::validation_failed, "SavePackage header package_hash mismatch");
    if (!package.integrity_manifest.sealed) return fail(ErrorCode::validation_failed, "SavePackage integrity not sealed");
    if (package.integrity_manifest.header_hash != expected.header_hash) return fail(ErrorCode::validation_failed, "Integrity header_hash mismatch");
    if (package.integrity_manifest.index_hash != expected.index_hash) return fail(ErrorCode::validation_failed, "Integrity index_hash mismatch");
    if (package.integrity_manifest.snapshot_hash != expected.snapshot_hash) return fail(ErrorCode::validation_failed, "Integrity snapshot_hash mismatch");
    if (package.integrity_manifest.event_log_hash != expected.event_log_hash) return fail(ErrorCode::validation_failed, "Integrity event_log_hash mismatch");
    if (package.integrity_manifest.command_log_hash != expected.command_log_hash) return fail(ErrorCode::validation_failed, "Integrity command_log_hash mismatch");
    if (package.integrity_manifest.random_log_hash != expected.random_log_hash) return fail(ErrorCode::validation_failed, "Integrity random_log_hash mismatch");
    if (package.integrity_manifest.trace_hash != expected.trace_hash) return fail(ErrorCode::validation_failed, "Integrity trace_hash mismatch");
    if (package.integrity_manifest.config_hash != expected.config_hash) return fail(ErrorCode::validation_failed, "Integrity config_hash mismatch");
    if (package.integrity_manifest.journal_hash != expected.journal_hash) return fail(ErrorCode::validation_failed, "Integrity journal_hash mismatch");
    if (package.integrity_manifest.package_hash != expected.package_hash) return fail(ErrorCode::validation_failed, "Integrity package_hash mismatch");

    for (auto required : {SaveSectionKind::Header, SaveSectionKind::StateSnapshot, SaveSectionKind::EventLog,
                          SaveSectionKind::CommandLog, SaveSectionKind::RandomLog, SaveSectionKind::PipelineTrace,
                          SaveSectionKind::ConfigManifest, SaveSectionKind::IntegrityManifest}) {
        if (!package.index.find(required)) return fail(ErrorCode::validation_failed, "SaveIndex missing section: " + toString(required));
    }
    const auto* header_entry = package.index.find(SaveSectionKind::Header);
    const auto* snapshot_entry = package.index.find(SaveSectionKind::StateSnapshot);
    const auto* event_entry = package.index.find(SaveSectionKind::EventLog);
    const auto* command_entry = package.index.find(SaveSectionKind::CommandLog);
    const auto* random_entry = package.index.find(SaveSectionKind::RandomLog);
    const auto* trace_entry = package.index.find(SaveSectionKind::PipelineTrace);
    const auto* config_entry = package.index.find(SaveSectionKind::ConfigManifest);
    if (header_entry->section_hash != expected.header_hash) return fail(ErrorCode::validation_failed, "SaveIndex header section_hash mismatch");
    if (snapshot_entry->section_hash != expected.snapshot_hash) return fail(ErrorCode::validation_failed, "SaveIndex snapshot section_hash mismatch");
    if (event_entry->section_hash != expected.event_log_hash) return fail(ErrorCode::validation_failed, "SaveIndex event section_hash mismatch");
    if (command_entry->section_hash != expected.command_log_hash) return fail(ErrorCode::validation_failed, "SaveIndex command section_hash mismatch");
    if (random_entry->section_hash != expected.random_log_hash) return fail(ErrorCode::validation_failed, "SaveIndex random section_hash mismatch");
    if (trace_entry->section_hash != expected.trace_hash) return fail(ErrorCode::validation_failed, "SaveIndex trace section_hash mismatch");
    if (config_entry->section_hash != expected.config_hash) return fail(ErrorCode::validation_failed, "SaveIndex config section_hash mismatch");
    return Result<void>::ok();
}

Result<void> RestorePlanStep::validateBasic() const {
    if (kind == RestorePlanStepKind::Unknown || kind == RestorePlanStepKind::TestOnly) return fail(ErrorCode::validation_enum_unknown, "RestorePlanStep kind invalid");
    auto key = validateSafeKey(step_key, "RestorePlanStep step_key"); if (key.is_error()) return key;
    return validateSafeKeys(reason_keys, "RestorePlanStep reason_keys");
}

Result<void> RestorePlan::validateBasic() const {
    auto save = validateIdValue(save_id.value(), "RestorePlan save_id"); if (save.is_error()) return save;
    auto snapshot = validateIdValue(selected_snapshot_id.value(), "RestorePlan selected_snapshot_id"); if (snapshot.is_error()) return snapshot;
    if (steps.empty()) return fail(ErrorCode::validation_failed, "RestorePlan steps empty");
    for (const auto& step : steps) { auto valid = step.validateBasic(); if (valid.is_error()) return valid; }
    return validateSafeKeys(warning_keys, "RestorePlan warning_keys");
}

Result<RestorePlan> SaveRestorePlanner::plan(const SaveGamePackage& package, StateVersion target_state_version) const {
    auto valid = SavePackageValidator{}.validate(package);
    if (valid.is_error()) return Result<RestorePlan>::fail(valid.errors());
    if (target_state_version < package.header.base_state_version || target_state_version > package.header.latest_state_version) {
        return Result<RestorePlan>::fail(makeError(ErrorCode::validation_failed, "RestorePlan target outside package range"));
    }
    const auto* snapshot = package.snapshot_section.latestAtOrBefore(target_state_version);
    if (!snapshot) return Result<RestorePlan>::fail(makeError(ErrorCode::validation_failed, "RestorePlan no snapshot at or before target"));

    RestorePlan plan;
    plan.save_id = package.header.save_id;
    plan.selected_snapshot_id = snapshot->snapshot_id;
    plan.target_state_version = target_state_version;
    auto add_step = [&](RestorePlanStepKind kind, std::string key) {
        RestorePlanStep step;
        step.kind = kind;
        step.step_key = std::move(key);
        step.target_state_version = target_state_version;
        step.reason_keys = {"p30_restore_plan"};
        plan.steps.push_back(std::move(step));
    };
    add_step(RestorePlanStepKind::VerifyIntegrity, "verify_integrity_manifest");
    add_step(RestorePlanStepKind::LoadConfigManifest, "load_config_manifest");
    add_step(RestorePlanStepKind::LoadSnapshot, "load_snapshot_" + snapshot->snapshot_id.value());
    add_step(RestorePlanStepKind::ApplyEventLog, "apply_event_log_until_target");
    add_step(RestorePlanStepKind::AttachReplayLogs, "attach_command_and_random_logs");
    add_step(RestorePlanStepKind::AttachTraceLog, "attach_pipeline_trace");
    add_step(RestorePlanStepKind::Ready, "restore_plan_ready");
    if (package.recovery_journal.has_value()) plan.warning_keys.push_back("package_has_recovery_journal");

    auto plan_valid = plan.validateBasic();
    if (plan_valid.is_error()) return Result<RestorePlan>::fail(plan_valid.errors());
    return Result<RestorePlan>::ok(std::move(plan));
}

} // namespace pathfinder::save
