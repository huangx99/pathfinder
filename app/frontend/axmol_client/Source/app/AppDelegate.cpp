#include "app/AppDelegate.h"
#include "scene/V3SandboxScene.h"

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
    auto* director = Director::getInstance();
    auto* render_view = director->getRenderView();
    if (!render_view) {
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)
        render_view = RenderViewImpl::createWithRect("Pathfinder V3 Sandbox", Rect(0, 0, kDesignWidth, kDesignHeight));
#else
        render_view = RenderViewImpl::create("Pathfinder V3 Sandbox");
#endif
        director->setRenderView(render_view);
    }

    director->setStatsDisplay(true);
    director->setAnimationInterval(1.0F / 60.0F);
    render_view->setDesignResolutionSize(kDesignWidth, kDesignHeight, ResolutionPolicy::SHOW_ALL);
    director->runWithScene(pf::client::V3SandboxScene::createScene());
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
}

void AppDelegate::applicationWillQuit() {}
