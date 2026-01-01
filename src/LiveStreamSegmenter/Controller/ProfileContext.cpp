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

#include "ProfileContext.hpp"

#include <curl/curl.h>

#include <obs-frontend-api.h>

#include <ILogger.hpp>

#include <AuthStore.hpp>
#include <EventHandlerStore.hpp>
#include <MultiLogger.hpp>
#include <ScriptingRuntime.hpp>
#include <StreamSegmenterDock.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeStore.hpp>

#include "YouTubeStreamSegmenterMainLoop.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

ProfileContext::ProfileContext(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
			       std::shared_ptr<Scripting::ScriptingRuntime> runtime,
			       std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
	: youTubeApiClient_(std::move(youTubeApiClient)),
	  runtime_(std::move(runtime)),
	  authStore_(std::make_shared<Store::AuthStore>()),
	  eventHandlerStore_(std::make_shared<Store::EventHandlerStore>()),
	  youTubeStore_(std::make_shared<Store::YouTubeStore>()),
	  dock_(dock),
	  logger_(std::make_shared<Logger::MultiLogger>(
		  std::vector<std::shared_ptr<const Logger::ILogger>>{logger, dock_->getLoggerAdapter()})),
	  youTubeStreamSegmenterMainLoop_(std::make_shared<YouTubeStreamSegmenterMainLoop>(
		  youTubeApiClient_, runtime_, authStore_, eventHandlerStore_, youTubeStore_, logger_, dock_))
{
	youTubeApiClient_->setLogger(logger_);
	authStore_->setLogger(logger_);
	eventHandlerStore_->setLogger(logger_);
	youTubeStore_->setLogger(logger_);

	authStore_->restoreAuthStore();
	eventHandlerStore_->restore();
	youTubeStore_->restoreYouTubeStore();

	dock_->setAuthStore(authStore_);
	dock_->setEventHandlerStore(eventHandlerStore_);
	dock_->setYouTubeStore(youTubeStore_);

	QObject::connect(dock_, &UI::StreamSegmenterDock::startButtonClicked, youTubeStreamSegmenterMainLoop_.get(),
			 &YouTubeStreamSegmenterMainLoop::startContinuousSession);

	QObject::connect(dock_, &UI::StreamSegmenterDock::stopButtonClicked, youTubeStreamSegmenterMainLoop_.get(),
			 &YouTubeStreamSegmenterMainLoop::stopContinuousSession);
	QObject::connect(dock_, &UI::StreamSegmenterDock::segmentNowButtonClicked,
			 youTubeStreamSegmenterMainLoop_.get(), &YouTubeStreamSegmenterMainLoop::segmentCurrentSession);

	youTubeStreamSegmenterMainLoop_->startMainLoop();
}

ProfileContext::~ProfileContext() noexcept = default;

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
