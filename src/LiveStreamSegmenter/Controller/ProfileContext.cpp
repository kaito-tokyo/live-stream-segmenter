/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#include "ProfileContext.hpp"

#include <curl/curl.h>

#include <obs-frontend-api.h>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/YouTubeApi/YouTubeApiClient.hpp>

#include <AuthStore.hpp>
#include <EventHandlerStore.hpp>
#include <YouTubeStore.hpp>
#include <ScriptingRuntime.hpp>
#include <StreamSegmenterDock.hpp>

#include "YouTubeStreamSegmenterMainLoop.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

ProfileContext::ProfileContext(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
			       std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
	: runtime_(runtime ? std::move(runtime) : throw std::invalid_argument("RuntimeIsNullError(ProfileContext)")),
	  dock_(dock ? dock : throw std::invalid_argument("DockIsNullError(ProfileContext)")),
	  authStore_(std::make_shared<Store::AuthStore>()),
	  eventHandlerStore_(std::make_shared<Store::EventHandlerStore>()),
	  youTubeStore_(std::make_shared<Store::YouTubeStore>()),
	  logger_(logger ? std::move(logger) : throw std::invalid_argument("LoggerIsNullError(ProfileContext)")),
	  youTubeStreamSegmenterMainLoop_(std::make_shared<YouTubeStreamSegmenterMainLoop>(
		  runtime_, authStore_, eventHandlerStore_, youTubeStore_, logger_, dock_))
{
	authStore_->setLogger(logger_);
	eventHandlerStore_->setLogger(logger_);
	youTubeStore_->setLogger(logger_);

	authStore_->restore();
	eventHandlerStore_->restore();
	youTubeStore_->restore();

	dock_->setAuthStore(authStore_);
	dock_->setEventHandlerStore(eventHandlerStore_);
	dock_->setYouTubeStore(youTubeStore_);

	QObject::connect(dock_, &UI::StreamSegmenterDock::startButtonClicked, youTubeStreamSegmenterMainLoop_.get(),
			 &YouTubeStreamSegmenterMainLoop::onStartContinuousSession);

	QObject::connect(dock_, &UI::StreamSegmenterDock::stopButtonClicked, youTubeStreamSegmenterMainLoop_.get(),
			 &YouTubeStreamSegmenterMainLoop::onStopContinuousSession);

	QObject::connect(dock_, &UI::StreamSegmenterDock::segmentNowButtonClicked,
			 youTubeStreamSegmenterMainLoop_.get(),
			 &YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession);

	QObject::connect(youTubeStreamSegmenterMainLoop_.get(), &YouTubeStreamSegmenterMainLoop::tick, dock_,
			 &UI::StreamSegmenterDock::onMainLoopTimerTick);

	youTubeStreamSegmenterMainLoop_->startMainLoop();
}

ProfileContext::~ProfileContext() noexcept = default;

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
