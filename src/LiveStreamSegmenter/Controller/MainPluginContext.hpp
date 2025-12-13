/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file 
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#pragma once

#include <QMainWindow>

#include <obs-frontend-api.h>

#include <ILogger.hpp>

#include <StreamSegmenterDock.hpp>

#include <AuthService.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class MainPluginContext {
public:
    MainPluginContext(std::shared_ptr<const Logger::ILogger> logger, QMainWindow *mainWindow) : logger_(std::move(logger)), dock_(new UI::StreamSegmenterDock(logger_, mainWindow)) {
        authService_.loadGoogleCredential();
        obs_frontend_add_dock_by_id("live_stream_segmenter_dock", obs_module_text("LiveStreamSegmenterDock"), dock_);
    }

    ~MainPluginContext() noexcept = default;

    MainPluginContext(const MainPluginContext &) = delete;
    MainPluginContext &operator=(const MainPluginContext &) = delete;
    MainPluginContext(MainPluginContext &&) = delete;
    MainPluginContext &operator=(MainPluginContext &&) = delete;

private:
    const std::shared_ptr<const Logger::ILogger> logger_;

    UI::StreamSegmenterDock *const dock_;
    Service::AuthService authService_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
