# P20 Knowledge Propagation Context Pack

## Purpose

P20 implements safe subjective knowledge propagation after P19 knowledge revision.

It lets one owner transfer a `KnowledgeClaim` to another owner as teaching-derived subjective knowledge. The recipient must receive its own claim, evidence chain, confidence, and source trace. P20 must never transfer hidden truth or raw object rules.

## Required Reading

- `doc/46_P20知识传播系统任务卡设计.md`
- `doc/45_P19知识冲突与修正任务卡设计.md`
- `doc/44_P18知识系统任务卡设计.md`
- `doc/14_传播系统设计.md`
- `doc/13_知识系统设计.md`
- `context_packs/knowledge_p19.md`
- `context_packs/knowledge_p18.md`

## Core Scope

Implement:

```text
KnowledgePropagationChannel
KnowledgePropagationDecision
KnowledgePropagationTrustBand
KnowledgePropagationFailureReason
KnowledgePropagationOptions
KnowledgePropagator
KnowledgePropagationTarget
KnowledgePropagationContext
KnowledgePropagationSourceClaim
KnowledgePropagationAttempt
KnowledgeTransferAssessment
KnowledgeRecipientClaimDraft
KnowledgePropagationPlan
KnowledgePropagationResult
KnowledgePropagationPlanner
KnowledgePropagationService
KnowledgePropagationApplier
```

Add tests:

```text
backend/tests/unit/knowledge/knowledge_propagation_test.cpp
backend/tests/integration/p20/knowledge_propagation_flow_test.cpp
backend/tests/integration/p20/knowledge_propagation_security_test.cpp
```

## Core Rules

- Propagate subjective knowledge, never truth.
- Recipient claim owner must be the target owner.
- Do not mutate or reuse the source claim as the recipient claim.
- Recipient evidence must include `KnowledgeEvidenceKind::Teaching`.
- Recipient confidence is computed from source confidence, channel weight, trust, distance, quality, and noise.
- `Deprecated` and `Disproven` claims are not propagated by default.
- Correction channel propagates valid corrected claims or correction patches, not raw disproven knowledge as action truth.
- P20 service must not directly write `KnowledgeRepository`.
- All `Result::ok(dto)` outputs must pass `validateBasic()`.

## Forbidden

Do not implement:

```text
Full tribe state
Morale / split risk / group combat
Civilization unlock
Object reactions
H5/API/HTTP/WebSocket/JSON
SaveManager
RL reward/done
AgentRuntime/Policy direct repository writes
Free-text AI knowledge generation
```

Do not read:

```text
ObjectDefinition hidden truth
GameState raw state
AgentTickRecord raw data
SaveGame
```

Do not copy:

```text
Source owner's full private MemoryRecord list
Source owner's full MemorySummary internals
Hidden truth reason keys
Deprecated/Disproven claims into ordinary recipient action knowledge
```

## Propagation Formula

Use deterministic first-version scoring:

```text
transfer_score = clamp(
    source_confidence
    * channel_weight
    * trust_score
    * distance_factor
    * channel_quality
    * (1.0 - noise_factor),
    0.0,
    1.0
)
```

Recipient confidence:

```text
created_confidence = min(transfer_score, options.max_created_confidence)
```

Status mapping:

```text
< 0.10 -> Candidate
0.10..0.34 -> Hypothesis
0.35..0.69 -> Shared
>= 0.70 -> Active
```

P20 should not create `Teachable` or `Operational` by default.

## Decision Priority

```text
invalid/security input -> Result::fail
no source claim -> Skipped
non-transferable source -> Failed
trust/channel score too low -> Failed
no matching target claim -> CreatedRecipientClaim
matching compatible target claim -> ReinforcedRecipientClaim / UpdatedRecipientClaim
matching conflicting target claim -> WeakenedRecipientClaim or route_to_revision
correction channel -> CorrectionDelivered when a valid correction is delivered
```

## Tests

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "knowledge|p18|p19|p20" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "ObjectDefinition hidden|edible_profile|hunger_delta|health_delta|effect_kind|true_trait|real_effect|object_truth|raw_state|hidden_truth|GameState|AgentTickRecord|SaveGame|SaveManager" backend/include/pathfinder/knowledge backend/src/knowledge backend/tests/integration/p20 backend/tests/unit/knowledge
rg -n "HTTP|WebSocket|nlohmann|json|SaveManager|reward_value|done\s*=|is_done|AgentRuntime|Policy|RulePipeline|GameState" backend/include/pathfinder/knowledge backend/src/knowledge backend/tests/integration/p20 backend/tests/unit/knowledge
```

The word `Propagation` is expected in P20 names; do not treat it as an external dependency by itself.

## Acceptance

P20 passes only if:

```text
Agent A can transfer an active/teachable/shared claim to Agent B.
Agent B receives its own valid KnowledgeClaim.
Recipient evidence is Teaching.
Recipient confidence is lower/equal to safe transfer limits.
Repository is not written by the service.
Deprecated/Disproven are blocked by default.
Hidden truth/raw state inputs fail.
All ok DTOs validate.
```

Minimum gameplay story:

```text
A knows red berry is edible.
A teaches B.
B now has a subjective, teaching-derived red berry edible claim.
B did not receive object truth.
```
