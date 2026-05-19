#include "pathfinder/danger/tribe_danger_bridge.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace pathfinder::danger;
using namespace pathfinder::foundation;

static void test_pressure_to_tribe_input() {
    TribeDangerPressureDraft a;
    a.tribe_id = TribeId("tribe_first");
    a.morale_delta = -0.2;
    a.trust_delta = -0.1;
    a.safety_pressure = 0.4;
    a.casualty_pressure = 0.3;
    a.split_risk_hint = 0.4;
    a.reason_keys = {"wolf_pressure"};
    TribeDangerPressureDraft b;
    b.tribe_id = TribeId("tribe_first");
    b.morale_delta = -0.1;
    b.safety_pressure = 0.5;
    b.reason_keys = {"rotten_food_pressure"};

    auto input = TribeDangerBridge{}.toTribeStateInput({a, b}, TribeId("tribe_first"), Tick(20));
    assert(input.is_ok());
    assert(std::fabs(input.value().morale_delta - (-0.3)) < 0.000001);
    assert(std::fabs(input.value().safety_pressure - 0.9) < 0.000001);
    assert(std::fabs(input.value().casualty_pressure - 0.4) < 0.000001);
    assert(input.value().reason_keys.size() == 3);
}

int main() {
    test_pressure_to_tribe_input();
    std::cout << "tribe_danger_bridge_test passed\n";
    return 0;
}
