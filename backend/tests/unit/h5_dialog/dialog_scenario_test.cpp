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

int main(int argc, char* argv[]) {
    test_default_scenario();
    test_find_object();
    test_find_feedback();
    std::cout << "All h5_dialog_scenario tests passed" << std::endl;
    return 0;
}
