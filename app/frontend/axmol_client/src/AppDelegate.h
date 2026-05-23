#pragma once

namespace pathfinder::axmol_client {

class AppDelegate final {
public:
    bool applicationDidFinishLaunching();
    void applicationDidEnterBackground();
    void applicationWillEnterForeground();
};

} // namespace pathfinder::axmol_client
