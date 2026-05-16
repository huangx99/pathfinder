#include "pathfinder/state/game_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;

void run_game_state_tests() {
    // Test valid GameState
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_001");
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(100);
        state.metadata.config_version = ConfigVersionId("config_v1");
        state.metadata.state_hash = HashValue(12345);

        auto result = state.validateBasic();
        assert(result.is_ok());
    }

    // Test empty state_id fails
    {
        GameState state;
        state.metadata.state_id = GameStateId("");
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(100);

        auto result = state.validateBasic();
        assert(result.is_error());
    }

    // Test invalid state_id format fails
    {
        GameState state;
        state.metadata.state_id = GameStateId("invalid id with spaces");
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(100);

        auto result = state.validateBasic();
        assert(result.is_error());
    }

    // Test cloneMetadataOnly
    {
        GameState state;
        state.metadata.state_id = GameStateId("test_state_004");
        state.metadata.state_version = StateVersion(5);
        state.metadata.current_tick = Tick(200);
        state.metadata.config_version = ConfigVersionId("config_v2");
        state.metadata.state_hash = HashValue(67890);

        GameState copy = state.cloneMetadataOnly();
        assert(copy.metadata.state_id == state.metadata.state_id);
        assert(copy.metadata.state_version == state.metadata.state_version);
        assert(copy.metadata.current_tick == state.metadata.current_tick);
        assert(copy.metadata.config_version == state.metadata.config_version);
        assert(copy.metadata.state_hash == state.metadata.state_hash);
    }

    std::cout << "game_state tests passed" << std::endl;
}
