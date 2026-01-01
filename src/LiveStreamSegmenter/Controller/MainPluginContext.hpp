/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * Live Stream Segmenter - Controller Module
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <mutex>

#include <QMainWindow>

#include <obs-frontend-api.h>

#include <ILogger.hpp>
#include <ScriptingRuntime.hpp>
#include <StreamSegmenterDock.hpp>
#include <CurlHandle.hpp>

#include "ProfileContext.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class MainPluginContext : public std::enable_shared_from_this<MainPluginContext> {
public:
	~MainPluginContext() noexcept;

	MainPluginContext(const MainPluginContext &) = delete;
	MainPluginContext &operator=(const MainPluginContext &) = delete;
	MainPluginContext(MainPluginContext &&) = delete;
	MainPluginContext &operator=(MainPluginContext &&) = delete;

	static std::shared_ptr<MainPluginContext> create(std::shared_ptr<const Logger::ILogger> logger,
							 QMainWindow *mainWindow);

private:
	MainPluginContext(std::shared_ptr<const Logger::ILogger> logger, QMainWindow *mainWindow);

	void registerFrontendEventCallback();

	static void handleFrontendEvent(enum obs_frontend_event event, void *private_data) noexcept;

	const std::shared_ptr<CurlHelper::CurlHandle> curl_;
	const std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient_;
	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	UI::StreamSegmenterDock *const dock_ = nullptr;
	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	std::shared_ptr<ProfileContext> profileContext_;
	std::weak_ptr<MainPluginContext> *handleFrontendEventWeakSelfPtr_ = nullptr;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
