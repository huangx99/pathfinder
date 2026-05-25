#include "pathfinder/camp/camp_runtime.h"
#include <memory>

namespace pathfinder::camp {

std::unique_ptr<CampRule> makeWaitTickRule();
std::unique_ptr<CampRule> makePlayerResourceRule();
std::unique_ptr<CampRule> makeKnowledgeCardRule();
std::unique_ptr<CampRule> makeNpcTaskRule();
std::unique_ptr<CampRule> makeScholarEventRule();

void registerCampCoreRules(CampRuntime& runtime) {
    runtime.registerRule(makeWaitTickRule());
    runtime.registerRule(makePlayerResourceRule());
    runtime.registerRule(makeKnowledgeCardRule());
    runtime.registerRule(makeNpcTaskRule());
    runtime.registerRule(makeScholarEventRule());
}

} // namespace pathfinder::camp
