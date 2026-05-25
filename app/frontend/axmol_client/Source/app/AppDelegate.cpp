#include "app/AppDelegate.h"
#include "scene/SandboxScene.h"
#include "pathfinder/logging/logger.h"

#include <cstdlib>
#include <string>

using namespace ax;

namespace {
constexpr float kDesignWidth = 1280.0F;
constexpr float kDesignHeight = 720.0F;

bool logEnabledFromEnvironment() {
    const char* value = std::getenv("PATHFINDER_CLIENT_LOG");
    if (!value) return true;
    const std::string text(value);
    return !(text == "0" || text == "false" || text == "FALSE" || text == "off" || text == "OFF");
}

std::string logPathFromEnvironment() {
    const char* value = std::getenv("PATHFINDER_CLIENT_LOG_PATH");
    return value ? std::string(value) : std::string();
}
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
    pathfinder::logging::Logger::instance().initialize(logEnabledFromEnvironment(), logPathFromEnvironment());
    pathfinder::logging::log(pathfinder::logging::tag::App, "application launching, log_file=" + pathfinder::logging::Logger::instance().logFilePath());

    auto* director = Director::getInstance();
    auto* render_view = director->getRenderView();
    if (!render_view) {
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)
        render_view = RenderViewImpl::createWithRect("Pathfinder Sandbox", Rect(0, 0, kDesignWidth, kDesignHeight));
#else
        render_view = RenderViewImpl::create("Pathfinder Sandbox");
#endif
        director->setRenderView(render_view);
        pathfinder::logging::log(pathfinder::logging::tag::App, "render view created");
    }

    director->setStatsDisplay(false);
    director->setAnimationInterval(1.0F / 60.0F);
    render_view->setDesignResolutionSize(kDesignWidth, kDesignHeight, ResolutionPolicy::SHOW_ALL);
    director->runWithScene(pf::client::SandboxScene::createScene());
    pathfinder::logging::log(pathfinder::logging::tag::App, "initial scene started");
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    pathfinder::logging::log(pathfinder::logging::tag::App, "application entered background");
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
    pathfinder::logging::log(pathfinder::logging::tag::App, "application entered foreground");
    Director::getInstance()->startAnimation();
}

void AppDelegate::applicationWillQuit() {
    pathfinder::logging::log(pathfinder::logging::tag::App, "application will quit");
    pathfinder::logging::Logger::instance().shutdown();
}
