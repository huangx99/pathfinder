#include "pathfinder/danger/threat_profile_builder.h"
#include "pathfinder/object/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::danger;
using namespace pathfinder::foundation;
using namespace pathfinder::reaction;
using pathfinder::object::ObjectCategory;

static ReactionRuntimeObject object(std::string key, std::vector<std::string> tags, std::vector<std::string> states) {
    ReactionRuntimeObject out;
    out.object_id = ObjectId("obj_" + key);
    out.definition_id = ObjectDefinitionId("def_" + key);
    out.object_key = std::move(key);
    out.category = ObjectCategory::Hazard;
    out.quantity = 1;
    out.tag_keys = std::move(tags);
    out.state_keys = std::move(states);
    out.location_key = "test_area";
    return out;
}

static void test_enum_roundtrip() {
    assert(dangerSourceKindFromString(toString(DangerSourceKind::Creature)).value() == DangerSourceKind::Creature);
    assert(hazardTriggerKindFromString(toString(HazardTriggerKind::Eat)).value() == HazardTriggerKind::Eat);
    assert(dangerSeverityFromString(toString(DangerSeverity::Severe)).value() == DangerSeverity::Severe);
    assert(dangerDecisionFromString(toString(DangerDecision::Avoided)).value() == DangerDecision::Avoided);
    assert(fearSignalKindFromString(toString(FearSignalKind::Alarm)).value() == FearSignalKind::Alarm);
    assert(countermeasureKindFromString(toString(CountermeasureKind::Tool)).value() == CountermeasureKind::Tool);
}

static void test_forbidden_keys() {
    assert(containsDangerForbiddenKey("hidden_truth_marker"));
    assert(containsDangerForbiddenKey("true_hp_value"));
    assert(containsDangerForbiddenKey("loot_box"));
    assert(!containsDangerForbiddenKey("rotten_food_risk"));
}

static void test_threat_profile_builder() {
    ThreatProfileBuilder builder;
    auto fire = builder.fromRuntimeObject(object("camp_fire", {"fire_source"}, {"burning"}));
    assert(fire.is_ok());
    assert(fire.value().source_kind == DangerSourceKind::ObjectReaction);
    assert(fire.value().base_severity == DangerSeverity::Harm);

    auto wolf = builder.fromCreatureProjection(EntityId("wolf_001"), "wolf", DangerSeverity::Severe, 0.8);
    assert(wolf.is_ok());
    assert(wolf.value().source_kind == DangerSourceKind::Creature);
    assert(wolf.value().base_severity == DangerSeverity::Severe);
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "enum" || mode == "all") test_enum_roundtrip();
    if (mode == "forbidden" || mode == "all") test_forbidden_keys();
    if (mode == "builder" || mode == "all") test_threat_profile_builder();
    std::cout << "danger_types_test passed\n";
    return 0;
}
