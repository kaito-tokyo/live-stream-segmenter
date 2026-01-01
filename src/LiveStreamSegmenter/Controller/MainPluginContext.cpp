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

#include "MainPluginContext.hpp"

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

MainPluginContext::~MainPluginContext() noexcept
{
	obs_frontend_remove_event_callback(handleFrontendEvent, handleFrontendEventWeakSelfPtr_);
	delete handleFrontendEventWeakSelfPtr_;
};

std::shared_ptr<MainPluginContext> MainPluginContext::create(std::shared_ptr<const Logger::ILogger> logger,
							     QMainWindow *mainWindow)
{
	auto self = std::shared_ptr<MainPluginContext>(new MainPluginContext(std::move(logger), mainWindow));
	self->registerFrontendEventCallback();
	return self;
}

namespace {

std::shared_ptr<const Logger::ILogger> composeLogger(std::shared_ptr<const Logger::ILogger> logger,
						     UI::StreamSegmenterDock *dock)
{
	if (!logger) {
		throw std::invalid_argument("LoggerIsNullError(composeLogger)");
	}
	if (!dock) {
		throw std::invalid_argument("DockIsNullError(composeLogger)");
	}
	return std::make_shared<Logger::MultiLogger>(
		std::vector<std::shared_ptr<const Logger::ILogger>>{std::move(logger), dock->getLoggerAdapter()});
}

} // anonymous namespace

MainPluginContext::MainPluginContext(std::shared_ptr<const Logger::ILogger> logger, QMainWindow *mainWindow)
	: curl_(std::make_shared<CurlHelper::CurlHandle>()),
	  youTubeApiClient_(std::make_shared<YouTubeApi::YouTubeApiClient>(curl_)),
	  runtime_(std::make_shared<Scripting::ScriptingRuntime>()),
	  dock_(new UI::StreamSegmenterDock(curl_, youTubeApiClient_, runtime_, mainWindow)),
	  logger_(composeLogger(std::move(logger), dock_))
{
	runtime_->setLogger(logger_);
	dock_->setLogger(logger_);

	obs_frontend_add_dock_by_id("live_stream_segmenter_dock", obs_module_text("LiveStreamSegmenterDock"), dock_);

	profileContext_ = std::make_shared<ProfileContext>(curl_, youTubeApiClient_, runtime_, logger_, dock_);
}

void MainPluginContext::registerFrontendEventCallback()
{
	handleFrontendEventWeakSelfPtr_ = new std::weak_ptr<MainPluginContext>(shared_from_this());
	obs_frontend_add_event_callback(handleFrontendEvent, handleFrontendEventWeakSelfPtr_);
}

void MainPluginContext::handleFrontendEvent(enum obs_frontend_event event, void *private_data) noexcept
{
	auto *weakSelfPtr = static_cast<std::weak_ptr<MainPluginContext> *>(private_data);

	if (auto self = weakSelfPtr->lock()) {
		try {
			if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGING) {
				std::scoped_lock lock(self->mutex_);
				self->profileContext_.reset();
			} else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
				std::scoped_lock lock(self->mutex_);
				self->profileContext_ =
					std::make_shared<ProfileContext>(self->curl_, self->youTubeApiClient_,
									 self->runtime_, self->logger_, self->dock_);
				self->logger_->info("ProfileChanged");
			}
		} catch (...) {
			self->logger_->error("UnhandledExceptionInFrontendEventCallback");
		}
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
