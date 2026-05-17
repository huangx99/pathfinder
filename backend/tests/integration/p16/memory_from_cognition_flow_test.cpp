#include "pathfinder/memory/memory_writer.h"
#include "pathfinder/memory/memory_store.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::memory;
using namespace pathfinder::foundation;
using namespace pathfinder::cognition;

void run_p16_memory_from_cognition_tests() {
    // ============================================================
    // CognitionEvidenceRecord -> MemoryCandidate -> ShortTerm
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_001");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "unknown_fruit";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Edibility;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
        evidence.observed_risk = CognitionRiskLevel::Low;
        evidence.confidence_weight = 0.4;
        evidence.source_event_id = EventId("evt_001");
        evidence.observed_tick = Tick(1);

        MemoryCandidateFactory factory;
        auto candidate_result = factory.fromCognitionEvidence(evidence);
        assert(candidate_result.is_ok());
        auto candidate = candidate_result.value();

        MemoryWriteService writer;
        auto write_result = writer.writeCandidate(candidate, {}, MemoryWriteOptions{});
        assert(write_result.is_ok());
        assert(write_result.value().decision == MemoryWriteDecision::Created);
        assert(write_result.value().created_records[0].scope == MemoryScope::ShortTerm);
    }

    // ============================================================
    // Repeated 3 times -> MidTerm
    // ============================================================
    {
        MemoryWriteService writer;
        MemoryWriteOptions opts;
        opts.short_to_mid_reinforcement_count = 3;

        std::vector<MemoryRecord> existing;

        for (size_t i = 0; i < 4; ++i) {
            CognitionEvidenceRecord evidence;
            evidence.evidence_id = CognitionEvidenceRecordId("ev_" + std::to_string(i));
            evidence.key.subject.kind = CognitionSubjectKind::Agent;
            evidence.key.subject.subject_id = EntityId("agent_001");
            evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
            evidence.key.target.target_id = "unknown_fruit";
            evidence.key.action_id = ActionId("eat");
            evidence.key.action_context = CognitionActionContextKind::Eat;
            evidence.key.aspect = CognitionAspect::Edibility;
            evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
            evidence.support = CognitionEvidenceSupport::Supports;
            evidence.outcome_signal = CognitionOutcomeSignal::NeedImproved;
            evidence.observed_risk = CognitionRiskLevel::Low;
            evidence.confidence_weight = 0.4;
            evidence.source_event_id = EventId("evt_" + std::to_string(i));
            evidence.observed_tick = Tick(static_cast<uint64_t>(i + 1));

            MemoryCandidateFactory factory;
            auto candidate = factory.fromCognitionEvidence(evidence).value();

            auto write_result = writer.writeCandidate(candidate, existing, opts);
            assert(write_result.is_ok());

            if (write_result.value().created_records.empty()) {
                existing = write_result.value().updated_records;
            } else {
                existing = write_result.value().created_records;
            }
        }

        assert(existing[0].scope == MemoryScope::MidTerm);
        assert(existing[0].reinforcement_count >= 3);
    }

    // ============================================================
    // Critical risk -> protected / LongTerm
    // ============================================================
    {
        CognitionEvidenceRecord evidence;
        evidence.evidence_id = CognitionEvidenceRecordId("ev_crit");
        evidence.key.subject.kind = CognitionSubjectKind::Agent;
        evidence.key.subject.subject_id = EntityId("agent_001");
        evidence.key.target.kind = CognitionTargetKind::ObjectDefinition;
        evidence.key.target.target_id = "poison_berry";
        evidence.key.action_id = ActionId("eat");
        evidence.key.action_context = CognitionActionContextKind::Eat;
        evidence.key.aspect = CognitionAspect::Risk;
        evidence.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        evidence.support = CognitionEvidenceSupport::Supports;
        evidence.outcome_signal = CognitionOutcomeSignal::DamageTaken;
        evidence.observed_risk = CognitionRiskLevel::Critical;
        evidence.confidence_weight = 0.8;
        evidence.source_event_id = EventId("evt_crit");
        evidence.observed_tick = Tick(1);

        MemoryCandidateFactory factory;
        auto candidate = factory.fromCognitionEvidence(evidence).value();

        MemoryWriteService writer;
        auto result = writer.writeCandidate(candidate, {}, MemoryWriteOptions{});
        assert(result.is_ok());
        assert(result.value().created_records[0].scope == MemoryScope::LongTerm);
        assert(result.value().created_records[0].protect_from_decay);
    }

    std::cout << "p16_memory_from_cognition tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_p16_memory_from_cognition_tests();
        return 0;
    }
    std::string mode(argv[1]);
    if (mode == "from_cognition" || mode == "repeated_evidence_promotes" || mode == "critical_risk_protected") {
        run_p16_memory_from_cognition_tests();
        return 0;
    }
    return 1;
}
