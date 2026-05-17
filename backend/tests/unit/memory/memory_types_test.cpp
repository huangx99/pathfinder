#include "pathfinder/memory/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;

void run_memory_types_tests() {
    // ============================================================
    // Enum roundtrips: MemoryOwnerKind
    // ============================================================
    {
        assert(toString(MemoryOwnerKind::Unknown) == "unknown");
        assert(toString(MemoryOwnerKind::Actor) == "actor");
        assert(toString(MemoryOwnerKind::Agent) == "agent");
        assert(toString(MemoryOwnerKind::Group) == "group");
        assert(toString(MemoryOwnerKind::Tribe) == "tribe");
        assert(toString(MemoryOwnerKind::Civilization) == "civilization");
        assert(toString(MemoryOwnerKind::TestOnly) == "test_only");

        assert(memoryOwnerKindFromString("actor").value() == MemoryOwnerKind::Actor);
        assert(memoryOwnerKindFromString("invalid").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemorySubjectKind
    // ============================================================
    {
        assert(toString(MemorySubjectKind::ObjectDefinition) == "object_definition");
        assert(toString(MemorySubjectKind::WorldObject) == "world_object");
        assert(memorySubjectKindFromString("object_definition").value() == MemorySubjectKind::ObjectDefinition);
        assert(memorySubjectKindFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryKind
    // ============================================================
    {
        assert(toString(MemoryKind::Experiment) == "experiment");
        assert(toString(MemoryKind::Success) == "success");
        assert(toString(MemoryKind::Failure) == "failure");
        assert(toString(MemoryKind::Warning) == "warning");
        assert(toString(MemoryKind::Accident) == "accident");
        assert(toString(MemoryKind::Discovery) == "discovery");
        assert(toString(MemoryKind::Contradiction) == "contradiction");
        assert(toString(MemoryKind::TeachingRelated) == "teaching_related");
        assert(memoryKindFromString("discovery").value() == MemoryKind::Discovery);
        assert(memoryKindFromString("xxx").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryScope
    // ============================================================
    {
        assert(toString(MemoryScope::ShortTerm) == "short_term");
        assert(toString(MemoryScope::MidTerm) == "mid_term");
        assert(toString(MemoryScope::LongTerm) == "long_term");
        assert(memoryScopeFromString("short_term").value() == MemoryScope::ShortTerm);
        assert(memoryScopeFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryLifecycle
    // ============================================================
    {
        assert(toString(MemoryLifecycle::Active) == "active");
        assert(toString(MemoryLifecycle::Fading) == "fading");
        assert(toString(MemoryLifecycle::Forgotten) == "forgotten");
        assert(toString(MemoryLifecycle::Protected) == "protected");
        assert(memoryLifecycleFromString("forgotten").value() == MemoryLifecycle::Forgotten);
        assert(memoryLifecycleFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryStrengthBand
    // ============================================================
    {
        assert(toString(MemoryStrengthBand::Trace) == "trace");
        assert(toString(MemoryStrengthBand::Weak) == "weak");
        assert(toString(MemoryStrengthBand::Stable) == "stable");
        assert(toString(MemoryStrengthBand::Strong) == "strong");
        assert(toString(MemoryStrengthBand::Core) == "core");
        assert(memoryStrengthBandFromString("core").value() == MemoryStrengthBand::Core);
        assert(memoryStrengthBandFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryImportance
    // ============================================================
    {
        assert(toString(MemoryImportance::Trivial) == "trivial");
        assert(toString(MemoryImportance::Normal) == "normal");
        assert(toString(MemoryImportance::Important) == "important");
        assert(toString(MemoryImportance::Critical) == "critical");
        assert(memoryImportanceFromString("critical").value() == MemoryImportance::Critical);
        assert(memoryImportanceFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemorySourceKind
    // ============================================================
    {
        assert(toString(MemorySourceKind::CognitionEvidence) == "cognition_evidence");
        assert(toString(MemorySourceKind::CognitionUpdate) == "cognition_update");
        assert(memorySourceKindFromString("cognition_evidence").value() == MemorySourceKind::CognitionEvidence);
        assert(memorySourceKindFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryWriteDecision
    // ============================================================
    {
        assert(toString(MemoryWriteDecision::Created) == "created");
        assert(toString(MemoryWriteDecision::Reinforced) == "reinforced");
        assert(toString(MemoryWriteDecision::Promoted) == "promoted");
        assert(memoryWriteDecisionFromString("promoted").value() == MemoryWriteDecision::Promoted);
        assert(memoryWriteDecisionFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryDecayDecision
    // ============================================================
    {
        assert(toString(MemoryDecayDecision::Decayed) == "decayed");
        assert(toString(MemoryDecayDecision::Faded) == "faded");
        assert(toString(MemoryDecayDecision::Forgotten) == "forgotten");
        assert(toString(MemoryDecayDecision::ProtectedSkipped) == "protected_skipped");
        assert(memoryDecayDecisionFromString("protected_skipped").value() == MemoryDecayDecision::ProtectedSkipped);
        assert(memoryDecayDecisionFromString("bad").is_error());
    }

    // ============================================================
    // Enum roundtrips: MemoryRejectReason
    // ============================================================
    {
        assert(toString(MemoryRejectReason::HiddenTruthDetected) == "hidden_truth_detected");
        assert(toString(MemoryRejectReason::RawStateDetected) == "raw_state_detected");
        assert(memoryRejectReasonFromString("hidden_truth_detected").value() == MemoryRejectReason::HiddenTruthDetected);
        assert(memoryRejectReasonFromString("bad").is_error());
    }

    // ============================================================
    // Helpers: strengthToBand
    // ============================================================
    {
        assert(strengthToBand(0.0) == MemoryStrengthBand::Unknown);
        assert(strengthToBand(0.05) == MemoryStrengthBand::Trace);
        assert(strengthToBand(0.30) == MemoryStrengthBand::Weak);
        assert(strengthToBand(0.50) == MemoryStrengthBand::Stable);
        assert(strengthToBand(0.80) == MemoryStrengthBand::Strong);
        assert(strengthToBand(0.95) == MemoryStrengthBand::Core);
    }

    // ============================================================
    // Hidden truth guard (BLOCKER-4: case-insensitive)
    // ============================================================
    {
        assert(containsMemoryForbiddenKey("edible_profile"));
        assert(containsMemoryForbiddenKey("hunger_delta"));
        assert(containsMemoryForbiddenKey("GameState"));
        assert(!containsMemoryForbiddenKey("safe_key"));
        assert(containsMemoryForbiddenKey(std::vector<std::string>{"safe", "edible_profile"}));

        // Case variants
        assert(containsMemoryForbiddenKey("gamestate"));
        assert(containsMemoryForbiddenKey("GAMESTATE"));
        assert(containsMemoryForbiddenKey("Health_Delta"));
        assert(containsMemoryForbiddenKey("HIDDEN_TRUTH"));
        assert(containsMemoryForbiddenKey("raw_state"));
        assert(containsMemoryForbiddenKey("SaveManager"));
    }

    std::cout << "memory_types tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_memory_types_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "types" || mode == "roundtrip") {
        run_memory_types_tests();
        return 0;
    }
    return 1;
}
