#pragma once

#include "axmol/axmol.h"

class AppDelegate final : private ax::Application {
public:
    AppDelegate();
    ~AppDelegate() override;

    void initContextAttrs() override;
    bool applicationDidFinishLaunching() override;
    void applicationDidEnterBackground() override;
    void applicationWillEnterForeground() override;
    void applicationWillQuit() override;
};
