#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/memory/memory_record.h"
#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/learning/learning_loop.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace pathfinder::h5_dialog {

// ============================================================
// Enums
// ============================================================

enum class DialogIntentKind {
    Unknown,
    Observe,
    TryEat,
    TryUse,
    RepeatLastAction,
    TeachRecipient,
    InspectActorKnowledge,
    InspectRecipientKnowledge,
    Help,
    Wait,
    Restart,
    TestOnly
};

std::string toString(DialogIntentKind kind);
pathfinder::foundation::Result<DialogIntentKind> dialogIntentKindFromString(const std::string& str);

enum class DialogActionKind {
    Unknown,
    Observe,
    Eat,
    Use,
    Teach,
    Inspect,
    Wait,
    Restart,
    TestOnly
};

std::string toString(DialogActionKind kind);
pathfinder::foundation::Result<DialogActionKind> dialogActionKindFromString(const std::string& str);

enum class DialogTurnDecision {
    Unknown,
    RepliedOnly,
    LearningLoopRan,
    TeachingRan,
    Rejected,
    Failed,
    TestOnly
};

std::string toString(DialogTurnDecision decision);
pathfinder::foundation::Result<DialogTurnDecision> dialogTurnDecisionFromString(const std::string& str);

enum class DialogObjectVisibility {
    Unknown,
    Visible,
    Mentioned,
    HiddenFromPlayer,
    TestOnly
};

std::string toString(DialogObjectVisibility visibility);
pathfinder::foundation::Result<DialogObjectVisibility> dialogObjectVisibilityFromString(const std::string& str);

// ============================================================
// Security Guard
// ============================================================

std::vector<std::string> dialogForbiddenKeys();
bool containsDialogForbiddenKey(const std::string& text);
bool containsDialogForbiddenKey(const std::vector<std::string>& values);

// ============================================================
// DTOs
// ============================================================

struct DialogRequestDto {
    std::string session_id;
    std::string input_text;
    uint64_t client_turn = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogIntent {
    DialogIntentKind kind = DialogIntentKind::Unknown;
    DialogActionKind action = DialogActionKind::Unknown;
    std::string object_key;
    std::string target_object_key;
    std::string recipient_key;
    std::string normalized_text;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogQuickActionDto {
    std::string label_text;
    std::string input_text;
    DialogIntentKind intent_kind = DialogIntentKind::Unknown;
    bool enabled = true;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogResponseDto {
    std::string session_id;
    DialogTurnDecision decision = DialogTurnDecision::Unknown;
    std::string reply_text;
    std::vector<std::string> state_summary_lines;
    std::vector<std::string> actor_knowledge_lines;
    std::vector<std::string> recipient_knowledge_lines;
    std::vector<DialogQuickActionDto> quick_actions;
    std::vector<std::string> debug_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogScenarioObject {
    std::string object_key;
    std::string display_name;
    std::string player_description;
    DialogObjectVisibility visibility = DialogObjectVisibility::Visible;
    std::vector<DialogActionKind> allowed_actions;
    std::vector<std::string> safe_tags;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogStateCondition {
    std::string object_key;
    std::string state_key;
    std::string op;
    double number_value = 0.0;
    std::string tag_value;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogStateMutation {
    std::string object_key;
    std::string state_key;
    std::string op;
    double number_value = 0.0;
    std::string tag_value;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogFeedbackTemplate {
    std::string feedback_key;
    std::string object_key;
    std::string target_object_key;
    DialogActionKind action = DialogActionKind::Unknown;
    std::string effect_key;
    std::vector<pathfinder::cognition::CognitionOutcomeSignal> outcome_signals;
    std::vector<std::string> condition_keys;
    std::vector<DialogStateCondition> state_conditions;
    std::vector<DialogStateMutation> state_mutations;
    double utility_delta = 0.0;
    double risk_delta = 0.0;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogScenario {
    std::string scenario_key;
    std::string display_name;
    std::string welcome_text;
    std::vector<DialogScenarioObject> objects;
    std::vector<DialogFeedbackTemplate> feedbacks;
    std::vector<std::string> quick_action_input_texts;
    std::vector<std::string> reason_keys;
};


struct DialogCausalExposureRecord {
    std::string object_key;
    std::string family_key;
    std::string action_key;
    std::string effect_key;
    uint64_t observed_tick = 0;
    double dose_amount = 1.0;
    std::vector<std::string> state_keys;
    std::vector<std::string> reason_keys;
};

struct DialogObjectRuntimeState {
    std::string object_key;
    std::unordered_map<std::string, double> numeric_states;
    std::vector<std::string> tag_states;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DialogSessionState {
    std::string session_id;
    std::string scenario_key;
    pathfinder::learning::LearningActorRef actor;
    pathfinder::learning::LearningActorRef recipient;
    uint64_t turn_index = 0;
    pathfinder::foundation::Tick current_tick;
    std::vector<std::string> visible_object_keys;
    std::optional<DialogIntent> last_learning_intent;

    std::vector<pathfinder::memory::MemoryRecord> actor_memories;
    std::vector<pathfinder::memory::MemorySummary> actor_summaries;
    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_claims;

    std::unordered_map<std::string, DialogObjectRuntimeState> object_runtime_states;
    std::vector<std::string> completed_action_keys;
    std::vector<DialogCausalExposureRecord> causal_exposures;
    std::vector<std::string> debug_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::h5_dialog
