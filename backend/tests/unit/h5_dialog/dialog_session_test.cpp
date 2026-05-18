#include "pathfinder/h5_dialog/dialog_session.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static void test_session_create() {
    InMemoryDialogSessionStore store;
    auto r = store.loadOrCreate("session_1");
    assert(r.is_ok());
    auto state = r.value();
    assert(state.session_id == "session_1");
    assert(!state.visible_object_keys.empty());
    std::cout << "session_create passed" << std::endl;
}

static void test_session_save_load() {
    InMemoryDialogSessionStore store;
    auto r1 = store.loadOrCreate("session_2");
    auto state = r1.value();
    state.turn_index = 5;
    auto save_r = store.save(state);
    assert(save_r.is_ok());
    auto r2 = store.loadOrCreate("session_2");
    assert(r2.value().turn_index == 5);
    std::cout << "session_save_load passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_session_create();
    test_session_save_load();
    std::cout << "All h5_dialog_session tests passed" << std::endl;
    return 0;
}
