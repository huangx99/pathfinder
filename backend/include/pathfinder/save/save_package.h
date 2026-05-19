#pragma once

#include "pathfinder/event/event_record.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/replay/command_replay.h"
#include "pathfinder/replay/random_replay.h"
#include "pathfinder/state/game_state.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::save {

struct SavePackageIdTag {};
struct SaveSnapshotIdTag {};
struct SaveTraceIdTag {};
struct SaveJournalIdTag {};
using SavePackageId = pathfinder::foundation::StrongId<SavePackageIdTag>;
using SaveSnapshotId = pathfinder::foundation::StrongId<SaveSnapshotIdTag>;
using SaveTraceId = pathfinder::foundation::StrongId<SaveTraceIdTag>;
using SaveJournalId = pathfinder::foundation::StrongId<SaveJournalIdTag>;

enum class SaveSectionKind {
    Unknown,
    Header,
    StateSnapshot,
    EventLog,
    CommandLog,
    RandomLog,
    PipelineTrace,
    ConfigManifest,
    IntegrityManifest,
    CrashRecoveryJournal,
    TestOnly
};

enum class RestorePlanStepKind {
    Unknown,
    VerifyIntegrity,
    LoadConfigManifest,
    LoadSnapshot,
    ApplyEventLog,
    AttachReplayLogs,
    AttachTraceLog,
    Ready,
    TestOnly
};

std::string toString(SaveSectionKind kind);
std::string toString(RestorePlanStepKind kind);
pathfinder::foundation::Result<SaveSectionKind> saveSectionKindFromString(const std::string& value);
pathfinder::foundation::Result<RestorePlanStepKind> restorePlanStepKindFromString(const std::string& value);

bool containsSaveForbiddenKey(const std::string& key);
bool containsSaveForbiddenKey(const std::vector<std::string>& keys);

struct SaveHeader {
    SavePackageId save_id;
    std::string save_format_version{"save_package.v1"};
    std::string game_build_version;
    pathfinder::foundation::GameId game_id;
    pathfinder::foundation::PlayerId player_id;
    pathfinder::foundation::ConfigVersionId config_version;
    pathfinder::foundation::Tick created_tick;
    pathfinder::foundation::StateVersion base_state_version;
    pathfinder::foundation::StateVersion latest_state_version;
    pathfinder::foundation::HashValue package_hash;

    pathfinder::foundation::Result<void> validateBasic(bool require_package_hash = true) const;
};

struct SaveIndexEntry {
    SaveSectionKind kind{SaveSectionKind::Unknown};
    std::string section_key;
    pathfinder::foundation::HashValue section_hash;
    size_t record_count{0};
    pathfinder::foundation::StateVersion from_state_version;
    pathfinder::foundation::StateVersion to_state_version;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct SaveIndex {
    std::vector<SaveIndexEntry> entries;

    const SaveIndexEntry* find(SaveSectionKind kind) const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StateSnapshotRecord {
    SaveSnapshotId snapshot_id;
    pathfinder::state::GameState snapshot;
    pathfinder::foundation::HashValue snapshot_hash;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StateSnapshotSection {
    std::vector<StateSnapshotRecord> snapshots;

    const StateSnapshotRecord* latestAtOrBefore(pathfinder::foundation::StateVersion target_version) const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct EventLogSection {
    std::vector<pathfinder::event::EventRecord> events;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CommandLogSection {
    pathfinder::replay::CommandReplayLog command_log;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct RandomLogSection {
    pathfinder::replay::RandomReplayLog random_log;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct PipelineTraceRecord {
    SaveTraceId trace_id;
    std::optional<pathfinder::foundation::CommandId> command_id;
    pathfinder::foundation::StateVersion state_version;
    std::vector<std::string> pipeline_stage_keys;
    std::vector<pathfinder::foundation::EventId> event_ids;
    std::vector<std::string> condition_trace_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct PipelineTraceSection {
    std::vector<PipelineTraceRecord> traces;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConfigVersionManifest {
    pathfinder::foundation::ConfigVersionId active_config_version;
    std::vector<pathfinder::foundation::ConfigVersionId> required_config_versions;
    pathfinder::foundation::HashValue config_content_hash;
    std::string compatibility_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CrashRecoveryJournal {
    SaveJournalId journal_id;
    std::string open_transaction_key;
    std::optional<pathfinder::foundation::CommandId> command_id;
    pathfinder::foundation::Tick started_tick;
    pathfinder::foundation::StateVersion state_version_before;
    std::string recovery_action_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct IntegrityManifest {
    pathfinder::foundation::HashValue header_hash;
    pathfinder::foundation::HashValue index_hash;
    pathfinder::foundation::HashValue snapshot_hash;
    pathfinder::foundation::HashValue event_log_hash;
    pathfinder::foundation::HashValue command_log_hash;
    pathfinder::foundation::HashValue random_log_hash;
    pathfinder::foundation::HashValue trace_hash;
    pathfinder::foundation::HashValue config_hash;
    pathfinder::foundation::HashValue journal_hash;
    pathfinder::foundation::HashValue package_hash;
    bool sealed{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct SaveGamePackage {
    SaveHeader header;
    SaveIndex index;
    StateSnapshotSection snapshot_section;
    EventLogSection event_log_section;
    CommandLogSection command_log_section;
    RandomLogSection random_log_section;
    PipelineTraceSection trace_section;
    ConfigVersionManifest config_manifest;
    IntegrityManifest integrity_manifest;
    std::optional<CrashRecoveryJournal> recovery_journal;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct SavePackageBuilderInput {
    SaveHeader header;
    StateSnapshotSection snapshot_section;
    EventLogSection event_log_section;
    CommandLogSection command_log_section;
    RandomLogSection random_log_section;
    PipelineTraceSection trace_section;
    ConfigVersionManifest config_manifest;
    std::optional<CrashRecoveryJournal> recovery_journal;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class SavePackageBuilder {
public:
    pathfinder::foundation::Result<SaveGamePackage> build(const SavePackageBuilderInput& input) const;
};

class SavePackageValidator {
public:
    pathfinder::foundation::Result<void> validate(const SaveGamePackage& package) const;
};

struct RestorePlanStep {
    RestorePlanStepKind kind{RestorePlanStepKind::Unknown};
    std::string step_key;
    pathfinder::foundation::StateVersion target_state_version;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct RestorePlan {
    SavePackageId save_id;
    SaveSnapshotId selected_snapshot_id;
    pathfinder::foundation::StateVersion target_state_version;
    std::vector<RestorePlanStep> steps;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class SaveRestorePlanner {
public:
    pathfinder::foundation::Result<RestorePlan> plan(
        const SaveGamePackage& package,
        pathfinder::foundation::StateVersion target_state_version) const;
};

pathfinder::foundation::HashValue calculateSavePackageHash(const SaveGamePackage& package);
IntegrityManifest calculateIntegrityManifest(const SaveGamePackage& package);

} // namespace pathfinder::save
