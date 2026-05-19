#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static void test_default_scenario() {
    DialogScenarioCatalog catalog;
    auto r = catalog.defaultScenario();
    assert(r.is_ok());
    auto scenario = r.value();
    assert(!scenario.objects.empty());
    assert(!scenario.feedbacks.empty());
    assert(!scenario.threat_knowledge_templates.empty());
    assert(scenario.threat_knowledge_templates.front().required_effect_key == "repel_beast");
    assert(!scenario.suggested_action_templates.empty());
    assert(scenario.default_knowledge_templates.empty());
    std::cout << "default_scenario passed" << std::endl;
}

static void test_find_object() {
    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    auto scenario = scenario_r.value();
    auto obj_r = catalog.findObject(scenario, "red_berry");
    assert(obj_r.is_ok());
    assert(obj_r.value()->object_key == "red_berry");
    std::cout << "find_object passed" << std::endl;
}

static void test_find_feedback() {
    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    auto scenario = scenario_r.value();
    auto fb_r = catalog.findFeedback(scenario, "red_berry", DialogActionKind::Eat);
    assert(fb_r.is_ok());
    assert(fb_r.value().effect_key == "restore_hunger");
    std::cout << "find_feedback passed" << std::endl;
}

static void test_template_key_validation() {
    DialogDefaultKnowledgeTemplate knowledge;
    knowledge.template_key = "bad.template.key";
    knowledge.owner_display_key = "companion";
    knowledge.subject_object_key = "torch";
    knowledge.action_key = "use";
    knowledge.effect_key = "repel_beast";
    knowledge.confidence = 0.8;
    assert(knowledge.validateBasic().is_error());

    DialogScenarioThreatKnowledgeTemplate threat;
    threat.template_key = "threat.beast.counter";
    threat.threat_object_key = "beast_shadow";
    threat.required_effect_key = "repel_beast";
    assert(threat.validateBasic().is_error());
    std::cout << "template_key_validation passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_default_scenario();
    test_find_object();
    test_find_feedback();
    test_template_key_validation();
    std::cout << "All h5_dialog_scenario tests passed" << std::endl;
    return 0;
}
