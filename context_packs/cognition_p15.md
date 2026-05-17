# Cognition P15 Context Pack: Formal Cognition Layer

## P15 Goal

P15 formalizes cognition after P3/P6 and before P16 memory:

```text
Turn minimal P3 CognitionClaim into a formal subjective cognition layer covering edibility, usability, effects, risk, utility, evidence, and confidence.
```

P15 is the first phase of the cognition-memory-knowledge-propagation main line.

## Strict Boundary

P15 does NOT implement:

```text
Memory system.
Short/Mid/Long term memory.
Memory decay or compression.
Knowledge system.
Knowledge propagation or teaching.
Tribe/civilization systems.
RL/LLM.
H5/HTTP/WebSocket.
SaveManager.
Full UseObjectResolver.
Object reactions or combinations.
ObjectRevealLevel, color, shape, visual reveal.
```

P15 must not read hidden truth:

```text
ObjectDefinition hidden truth.
edible_profile.
hunger_delta / health_delta as truth fields.
effect_kind as object truth.
SaveGame.
```

P15 may consume only safe feedback outputs:

```text
FeedbackGenerated events.
ActionResolved events.
Observable StateChange before/after results.
Legacy P3 CognitionEvidence as feedback compatibility input.
Test-only safe CognitionFeedbackSignal.
```

## Compatibility Rule

Do not delete or replace P3 types:

```text
CognitionEffectKind
CognitionKey
CognitionEvidence
CognitionClaim
CognitionState
CognitionUpdater
```

Add V2 in parallel:

```text
CognitionClaimV2
CognitionEvidenceRecord
CognitionStateV2
CognitionConfidenceModel
CognitionQueryService
CognitionEvidenceBuilder
CognitionUpdaterV2
```

Recommended GameState shape:

```cpp
pathfinder::cognition::CognitionState cognition_state;      // P3 legacy
pathfinder::cognition::CognitionStateV2 cognition_state_v2; // P15 formal layer
```

P3/P7 tests must keep passing.

## New Files

```text
backend/include/pathfinder/cognition/cognition_v2_types.h
backend/src/cognition/cognition_v2_types.cpp
backend/include/pathfinder/cognition/cognition_v2_state.h
backend/src/cognition/cognition_v2_state.cpp
backend/include/pathfinder/cognition/cognition_confidence.h
backend/src/cognition/cognition_confidence.cpp
backend/include/pathfinder/cognition/cognition_query.h
backend/src/cognition/cognition_query.cpp
backend/include/pathfinder/cognition/cognition_evidence_builder.h
backend/src/cognition/cognition_evidence_builder.cpp
backend/tests/unit/cognition/cognition_v2_types_test.cpp
backend/tests/unit/cognition/cognition_confidence_model_test.cpp
backend/tests/unit/cognition/cognition_query_service_test.cpp
backend/tests/unit/cognition/cognition_v2_update_test.cpp
backend/tests/integration/p15/cognition_eat_feedback_flow_test.cpp
backend/tests/integration/p15/cognition_use_feedback_flow_test.cpp
backend/tests/integration/p15/cognition_hidden_truth_boundary_test.cpp
context_packs/cognition_p15.md
```

## Allowed Modifications

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/state/game_state.h
backend/src/state/game_state.cpp
backend/src/pipeline/rule_pipeline.cpp
backend/tests/integration/p3/unknown_fruit_eat_flow_test.cpp
backend/tests/integration/p7/agent_runtime_unknown_fruit_flow_test.cpp
```

Do not modify AgentRuntime/Policy to write cognition.

## Required Enums

```cpp
enum class CognitionSubjectKind {
    Unknown,
    Actor,
    Agent,
    Group,
    Tribe,
    Civilization,
    TestOnly
};

enum class CognitionTargetKind {
    Unknown,
    ObjectDefinition,
    WorldObject,
    ObjectCategory,
    AgentSpecies,
    EnvironmentFeature,
    TestOnly
};

enum class CognitionAspect {
    Unknown,
    Edibility,
    Usability,
    ConsumptionEffect,
    UseEffect,
    Risk,
    Utility
};

enum class CognitionBeliefPolarity {
    Unknown,
    Positive,
    Negative,
    Mixed
};

enum class CognitionEvidenceSupport {
    Unknown,
    Supports,
    Refutes,
    Neutral,
    Conflicts
};

enum class CognitionEvidenceSourceKind {
    Unknown,
    DirectActionFeedback,
    ObservedEvent,
    StateChangeObservation,
    TaughtByOther,
    ImportedSafeProjection,
    TestOnly
};

enum class CognitionActionContextKind {
    Unknown,
    Eat,
    Use,
    Touch,
    Combine,
    Observe,
    TestOnly
};

enum class CognitionOutcomeSignal {
    Unknown,
    NeedImproved,
    NeedWorsened,
    HealthImproved,
    HealthWorsened,
    DamageTaken,
    ObjectConsumed,
    ToolActivated,
    NewObjectProduced,
    NoVisibleEffect,
    DangerObserved,
    TestOnly
};

enum class CognitionRiskLevel {
    Unknown,
    None,
    Low,
    Medium,
    High,
    Critical
};

enum class CognitionConfidenceBand {
    Unknown,
    Untrusted,
    Hypothesis,
    Actionable,
    Reliable,
    Teachable
};

enum class CognitionUpdateDecision {
    Unknown,
    Created,
    Reinforced,
    Weakened,
    Conflicted,
    Ignored
};

enum class CognitionQueryMode {
    Unknown,
    ExactTarget,
    IncludeTargetKindFallback,
    BestActionable,
    TeachableCandidates
};
```

All enums need stable `toString/fromString` tests.

## Required DTOs

```cpp
struct CognitionSubject {
    CognitionSubjectKind kind = CognitionSubjectKind::Unknown;
    pathfinder::foundation::EntityId subject_id;
    std::optional<pathfinder::foundation::EntityId> owner_group_id;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionTarget {
    CognitionTargetKind kind = CognitionTargetKind::Unknown;
    std::string target_id;
    std::optional<std::string> category_id;
    std::string public_label_key;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionClaimKeyV2 {
    CognitionSubject subject;
    CognitionTarget target;
    pathfinder::foundation::ActionId action_id;
    CognitionActionContextKind action_context = CognitionActionContextKind::Unknown;
    CognitionAspect aspect = CognitionAspect::Unknown;
    bool operator==(const CognitionClaimKeyV2& other) const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionEvidenceRecordIdTag {};
using CognitionEvidenceRecordId = pathfinder::foundation::StrongId<CognitionEvidenceRecordIdTag>;

struct CognitionClaimV2IdTag {};
using CognitionClaimV2Id = pathfinder::foundation::StrongId<CognitionClaimV2IdTag>;

struct CognitionEvidenceRecord {
    CognitionEvidenceRecordId evidence_id;
    CognitionClaimKeyV2 key;
    CognitionEvidenceSourceKind source_kind = CognitionEvidenceSourceKind::Unknown;
    CognitionEvidenceSupport support = CognitionEvidenceSupport::Unknown;
    CognitionOutcomeSignal outcome_signal = CognitionOutcomeSignal::Unknown;
    CognitionRiskLevel observed_risk = CognitionRiskLevel::Unknown;
    double confidence_weight = 0.0;
    double utility_signal = 0.0;
    pathfinder::foundation::EventId source_event_id;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionClaimV2 {
    CognitionClaimV2Id claim_id;
    CognitionClaimKeyV2 key;
    CognitionBeliefPolarity polarity = CognitionBeliefPolarity::Unknown;
    double confidence = 0.0;
    CognitionConfidenceBand confidence_band = CognitionConfidenceBand::Unknown;
    CognitionRiskLevel risk_level = CognitionRiskLevel::Unknown;
    double utility_score = 0.0;
    size_t evidence_count = 0;
    size_t supporting_evidence_count = 0;
    size_t refuting_evidence_count = 0;
    bool conflicted = false;
    std::optional<CognitionEvidenceRecordId> first_evidence_id;
    std::optional<CognitionEvidenceRecordId> last_evidence_id;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;
    pathfinder::foundation::Result<void> validateBasic() const;
};
```

## State And Services

```cpp
class CognitionStateV2 {
public:
    const std::vector<CognitionClaimV2>& claims() const;
    const std::vector<CognitionEvidenceRecord>& evidence_records() const;
    const CognitionClaimV2* findClaim(const CognitionClaimKeyV2& key) const;
    const CognitionClaimV2* findById(const CognitionClaimV2Id& claim_id) const;
    void upsertClaim(CognitionClaimV2 claim);
    pathfinder::foundation::Result<void> appendEvidence(CognitionEvidenceRecord evidence);
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionConfidenceModelOptions {
    double initial_direct_feedback_confidence = 0.35;
    double reinforce_multiplier = 0.65;
    double refute_multiplier = 0.75;
    double conflict_penalty = 0.20;
    double min_confidence = 0.0;
    double max_confidence = 1.0;
    double actionable_threshold = 0.50;
    double reliable_threshold = 0.70;
    double teachable_threshold = 0.85;
    pathfinder::foundation::Result<void> validateBasic() const;
};

class CognitionConfidenceModel {
public:
    CognitionConfidenceBand bandFor(double confidence, const CognitionConfidenceModelOptions& options = {}) const;
    pathfinder::foundation::Result<CognitionUpdateResult> applyEvidence(const CognitionUpdateRequest& request) const;
};

class CognitionUpdaterV2 {
public:
    pathfinder::foundation::Result<CognitionUpdateResult> applyEvidence(
        CognitionStateV2& state,
        const CognitionEvidenceRecord& evidence,
        const CognitionConfidenceModelOptions& options = {}) const;
};
```

## Evidence Builder

```cpp
struct CognitionFeedbackSignal {
    CognitionSubject subject;
    CognitionTarget target;
    pathfinder::foundation::ActionId action_id;
    CognitionActionContextKind action_context = CognitionActionContextKind::Unknown;
    CognitionOutcomeSignal outcome_signal = CognitionOutcomeSignal::Unknown;
    CognitionRiskLevel risk_level = CognitionRiskLevel::Unknown;
    double utility_signal = 0.0;
    pathfinder::foundation::EventId source_event_id;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;
    pathfinder::foundation::Result<void> validateBasic() const;
};

class CognitionEvidenceBuilder {
public:
    pathfinder::foundation::Result<std::vector<CognitionEvidenceRecord>> fromFeedbackSignal(
        const CognitionFeedbackSignal& signal) const;

    pathfinder::foundation::Result<std::vector<CognitionEvidenceRecord>> fromLegacyEvidence(
        const CognitionEvidence& legacy_evidence) const;
};
```

Mapping rules:

```text
Eat + NeedImproved -> Edibility Positive, ConsumptionEffect Positive, Risk Negative/None, Utility Positive.
Eat + HealthWorsened/DamageTaken -> Risk Positive, ConsumptionEffect Negative, Utility Negative.
Use + ToolActivated -> Usability Positive, UseEffect Positive, Utility Positive.
Use + NewObjectProduced -> Usability Positive, UseEffect Positive.
Use + NoVisibleEffect -> Usability Negative or low-confidence Neutral.
DangerObserved -> Risk Positive.
```

`fromLegacyEvidence` is only for P3 compatibility. It may normalize legacy observed feedback into V2 outcome signals, but must not write legacy hidden field names into V2 DTOs.

## Query Service

```cpp
class CognitionQueryService {
public:
    pathfinder::foundation::Result<CognitionQueryResult> query(
        const CognitionStateV2& state,
        const CognitionQuery& query) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findBestClaim(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target,
        CognitionAspect aspect) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findEdibility(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findUsability(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;

    pathfinder::foundation::Result<std::optional<CognitionClaimV2>> findRisk(
        const CognitionStateV2& state,
        const CognitionSubject& subject,
        const CognitionTarget& target) const;
};
```

QueryService is read-only and must not read ObjectDefinition.

## Pipeline Integration

Recommended P15 pipeline:

```text
RuleResolver produces legacy P3 CognitionEvidence.
RulePipeline applies P3 CognitionUpdater unchanged.
RulePipeline calls CognitionEvidenceBuilder::fromLegacyEvidence.
RulePipeline applies CognitionUpdaterV2 into GameState.cognition_state_v2.
```

Do not make EatObjectResolver directly construct V2 evidence unless necessary.
Do not implement UseObjectResolver in P15.

## Required Tests

Unit tests:

```text
All P15 enum roundtrips.
All P15 DTO validateBasic.
CognitionStateV2 find/upsert/append/validate.
Confidence model create/reinforce/refute/conflict/band.
EvidenceBuilder fromFeedbackSignal for Eat and Use.
EvidenceBuilder fromLegacyEvidence without leaking hidden field names.
QueryService exact/best/edibility/usability/risk/teachable.
```

Integration tests:

```text
UnknownFruitFixture eat flow creates legacy P3 claim and P15 V2 claims.
Eat flow creates Edibility and ConsumptionEffect or Utility claim.
Eat flow creates Risk claim without reading ObjectDefinition hidden truth.
Use feedback signal creates Usability and UseEffect claims without full UseObjectResolver.
Hidden truth keys in reason_keys/public_label_key are rejected.
```

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "cognition_v2|p15" --output-on-failure
ctest --test-dir build/backend -R "cognition|p15|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|pipeline|rules|state|event|agent" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scans:

```bash
rg -n "ObjectRevealLevel|visual_reveal|color|shape" \
  backend/include/pathfinder/cognition \
  backend/src/cognition \
  backend/tests/unit/cognition \
  backend/tests/integration/p15 \
  && exit 1 || true

rg -n "edible_profile|hunger_delta|health_delta|effect_kind|ObjectDefinition|GameState|SaveGame|SaveManager" \
  backend/include/pathfinder/cognition/cognition_v2_types.h \
  backend/include/pathfinder/cognition/cognition_v2_state.h \
  backend/include/pathfinder/cognition/cognition_confidence.h \
  backend/include/pathfinder/cognition/cognition_query.h \
  backend/include/pathfinder/cognition/cognition_evidence_builder.h \
  backend/src/cognition/cognition_v2_types.cpp \
  backend/src/cognition/cognition_v2_state.cpp \
  backend/src/cognition/cognition_confidence.cpp \
  backend/src/cognition/cognition_query.cpp \
  backend/src/cognition/cognition_evidence_builder.cpp \
  backend/tests/integration/p15 \
  | rg -v "hidden_truth|forbidden|legacy|fromLegacyEvidence|RejectsHiddenTruth" \
  && exit 1 || true
```

## Completion Criteria

P15 is complete only when:

```text
Cognition V2 types, state, confidence model, updater, evidence builder, and query service exist.
P3 legacy cognition still passes.
Eat flow produces V2 Edibility/Risk/ConsumptionEffect or Utility cognition.
Use feedback produces V2 Usability/UseEffect cognition.
No ObjectDefinition hidden truth is read by V2 cognition services.
No visual reveal/color/shape cognition is introduced.
P15 targeted tests pass.
Related regression tests pass.
Full backend tests pass.
```

## Implementation Order

```text
1. Implement cognition_v2_types.
2. Implement CognitionStateV2.
3. Implement CognitionConfidenceModel.
4. Implement CognitionUpdaterV2.
5. Implement CognitionEvidenceBuilder.
6. Implement CognitionQueryService.
7. Add GameState.cognition_state_v2 and RulePipeline legacy evidence bridge.
8. Add P15 unit/integration/security tests.
9. Update CMake and dev_notes.
10. Run verification commands and boundary scans.
```
