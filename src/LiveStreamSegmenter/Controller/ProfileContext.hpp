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

#include <curl/curl.h>

#include <obs-frontend-api.h>

#include <ILogger.hpp>

#include <AuthStore.hpp>
#include <CurlHandle.hpp>
#include <EventHandlerStore.hpp>
#include <MultiLogger.hpp>
#include <ScriptingRuntime.hpp>
#include <StreamSegmenterDock.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeStore.hpp>

#include "YouTubeStreamSegmenterMainLoop.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class ProfileContext {
public:
	ProfileContext(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
		       std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock);

	~ProfileContext() noexcept;

	ProfileContext(const ProfileContext &) = delete;
	ProfileContext &operator=(const ProfileContext &) = delete;
	ProfileContext(ProfileContext &&) = delete;
	ProfileContext &operator=(ProfileContext &&) = delete;

private:
	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	UI::StreamSegmenterDock *const dock_;

	const std::shared_ptr<Store::AuthStore> authStore_;
	const std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	const std::shared_ptr<Store::YouTubeStore> youTubeStore_;

	const std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<YouTubeStreamSegmenterMainLoop> youTubeStreamSegmenterMainLoop_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
