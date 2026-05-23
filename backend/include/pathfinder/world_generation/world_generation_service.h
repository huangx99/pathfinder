#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include <memory>
#include <set>

namespace pathfinder::content { class ContentRegistry; }

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// WorldGenerationService - orchestrates terrain, resource, and safety generation
// ---------------------------------------------------------------------------

class WorldGenerationService {
public:
    WorldGenerationService();

    // Main generation entry point
    WorldGenerationResult generate(const WorldGenerationRequest& request);

    // Load or define a profile. Content-backed profiles override terrain-only fallbacks.
    void registerProfile(const WorldgenProfile& profile);
    pathfinder::foundation::Result<void> registerContentProfiles(const pathfinder::content::ContentRegistry& registry);
    const WorldgenProfile* findProfile(const std::string& profile_key) const;

    // Check if a region was already generated (stub for P46, expanded in P54)
    bool isRegionGenerated(const std::string& world_id, const WorldRegionCoord& coord) const;
    void markRegionGenerated(const std::string& world_id, const WorldRegionCoord& coord);

private:
    std::map<std::string, WorldgenProfile> profiles_;
    std::map<std::string, std::set<WorldRegionCoord>> generated_regions_;

    // Built-in minimal profiles for P46
    void registerBuiltinProfiles();
};

} // namespace pathfinder::world_generation
