#include "AppDelegate.h"
#include "axmol/axmol.h"

int axmol_main() {
    AppDelegate app;
    return ax::Application::getInstance()->run();
}

int main(int, char**) {
    const auto result = axmol_main();
#if AX_OBJECT_LEAK_DETECTION
    ax::Object::printLeaks();
#endif
    return result;
}
