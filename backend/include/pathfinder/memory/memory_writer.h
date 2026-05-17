#pragma once

#include "pathfinder/memory/memory_record.h"
#include "pathfinder/cognition/cognition_v2_types.h"

namespace pathfinder::memory {

// ============================================================
// MemoryCandidateFactory
// ============================================================

class MemoryCandidateFactory {
public:
    pathfinder::foundation::Result<MemoryCandidate> fromCognitionUpdate(
        const pathfinder::cognition::CognitionUpdateResult& update_result,
        const pathfinder::cognition::CognitionEvidenceRecord& evidence) const;

    pathfinder::foundation::Result<MemoryCandidate> fromCognitionEvidence(
        const pathfinder::cognition::CognitionEvidenceRecord& evidence) const;
};

// ============================================================
// MemoryIdFactory
// ============================================================

class MemoryIdFactory {
public:
    pathfinder::foundation::Result<pathfinder::foundation::MemoryId> makeMemoryId(
        const MemoryOwner& owner,
        const MemorySubject& subject,
        const MemoryEvidenceRef& evidence_ref,
        size_t sequence_index) const;
};

// ============================================================
// MemoryWriteService
// ============================================================

class MemoryWriteService {
public:
    pathfinder::foundation::Result<MemoryWriteResult> writeCandidate(
        const MemoryCandidate& candidate,
        const std::vector<MemoryRecord>& existing_records,
        const MemoryWriteOptions& options = {}) const;
};

} // namespace pathfinder::memory
