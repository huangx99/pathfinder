#include "AppDelegate.h"

int main() {
    pathfinder::axmol_client::AppDelegate app;
    return app.applicationDidFinishLaunching() ? 0 : 1;
}
