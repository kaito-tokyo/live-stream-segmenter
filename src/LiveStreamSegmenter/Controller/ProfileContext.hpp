/*
 * Live Stream Segmenter - Controller Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include <obs-frontend-api.h>

#include <ILogger.hpp>

#include <AuthStore.hpp>
#include <EventHandlerStore.hpp>
#include <YouTubeStore.hpp>
#include <StreamSegmenterDock.hpp>

#include "YouTubeStreamSegmenterMainLoop.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class ProfileContext {
public:
	ProfileContext(std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
		: ProfileContext(std::make_shared<Scripting::ScriptingRuntime>(logger), logger, dock)
	{
	}

	ProfileContext(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
		       std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
		: logger_(std::move(logger)),
		  runtime_(std::move(runtime)),
		  authStore_(std::make_shared<Store::AuthStore>(logger_)),
		  eventHandlerStore_(std::make_shared<Store::EventHandlerStore>(logger_)),
		  youTubeStore_(std::make_shared<Store::YouTubeStore>(logger_)),
		  dock_(dock),
		  youTubeStreamSegmenterMainLoop_(std::make_shared<YouTubeStreamSegmenterMainLoop>(
			  runtime_, authStore_, eventHandlerStore_, logger_, dock_))
	{
		authStore_->restoreAuthStore();
		eventHandlerStore_->restore();
		youTubeStore_->restoreYouTubeStore();

		dock_->setAuthStore(authStore_);
		dock_->setEventHandlerStore(eventHandlerStore_);
		dock_->setYouTubeStore(youTubeStore_);

		QObject::connect(dock_, &UI::StreamSegmenterDock::startButtonClicked,
				 youTubeStreamSegmenterMainLoop_.get(),
				 &YouTubeStreamSegmenterMainLoop::startContinuousSession);

		QObject::connect(dock_, &UI::StreamSegmenterDock::stopButtonClicked,
				 youTubeStreamSegmenterMainLoop_.get(),
				 &YouTubeStreamSegmenterMainLoop::stopContinuousSession);
		QObject::connect(dock_, &UI::StreamSegmenterDock::segmentNowButtonClicked,
				 youTubeStreamSegmenterMainLoop_.get(),
				 &YouTubeStreamSegmenterMainLoop::segmentCurrentSession);

		youTubeStreamSegmenterMainLoop_->startMainLoop();
	}

	~ProfileContext() = default;

	ProfileContext(const ProfileContext &) = delete;
	ProfileContext &operator=(const ProfileContext &) = delete;
	ProfileContext(ProfileContext &&) = delete;
	ProfileContext &operator=(ProfileContext &&) = delete;

private:
	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	const std::shared_ptr<Store::AuthStore> authStore_;
	const std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	const std::shared_ptr<Store::YouTubeStore> youTubeStore_;

	UI::StreamSegmenterDock *const dock_;
	std::shared_ptr<YouTubeStreamSegmenterMainLoop> youTubeStreamSegmenterMainLoop_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
