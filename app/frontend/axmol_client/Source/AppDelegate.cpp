#include "AppDelegate.h"
#include "MainScene.h"

using namespace ax;

namespace {
constexpr float kDesignWidth = 1280.0F;
constexpr float kDesignHeight = 720.0F;
}

AppDelegate::AppDelegate() = default;
AppDelegate::~AppDelegate() = default;

void AppDelegate::initContextAttrs() {
    ContextAttrs attrs = {};
    attrs.powerPreference = PowerPreference::HighPerformance;
#if AX_TARGET_PLATFORM != AX_PLATFORM_WIN32
    attrs.renderScaleMode = RenderScaleMode::Physical;
#endif
    setContextAttrs(attrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    auto director = Director::getInstance();
    auto renderView = director->getRenderView();
    if (!renderView) {
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)
        renderView = RenderViewImpl::createWithRect(
            "Pathfinder Axmol Client",
            Rect(0, 0, kDesignWidth, kDesignHeight));
#else
        renderView = RenderViewImpl::create("Pathfinder Axmol Client");
#endif
        director->setRenderView(renderView);
    }

    director->setStatsDisplay(true);
    director->setAnimationInterval(1.0F / 60.0F);
    renderView->setDesignResolutionSize(kDesignWidth, kDesignHeight, ResolutionPolicy::SHOW_ALL);

    FileUtils::getInstance()->addSearchPath("Content");

    director->runWithScene(MainScene::create());
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
}

void AppDelegate::applicationWillQuit() {
}
