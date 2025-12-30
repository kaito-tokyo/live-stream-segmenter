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

#include <memory>
#include <mutex>

#include <QMainWindow>

#include <obs-frontend-api.h>

#include <ILogger.hpp>
#include <ScriptingRuntime.hpp>
#include <StreamSegmenterDock.hpp>

#include "ProfileContext.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class MainPluginContext : public std::enable_shared_from_this<MainPluginContext> {
public:
	~MainPluginContext() noexcept
	{
		if (handleFrontendEventWeakSelfPtr_) {
			obs_frontend_remove_event_callback(handleFrontendEvent, handleFrontendEventWeakSelfPtr_);
			delete handleFrontendEventWeakSelfPtr_;
		}
	};

	MainPluginContext(const MainPluginContext &) = delete;
	MainPluginContext &operator=(const MainPluginContext &) = delete;
	MainPluginContext(MainPluginContext &&) = delete;
	MainPluginContext &operator=(MainPluginContext &&) = delete;

	static std::shared_ptr<MainPluginContext> create(std::shared_ptr<const Logger::ILogger> logger,
							 QMainWindow *mainWindow)
	{
		auto self = std::shared_ptr<MainPluginContext>(new MainPluginContext(std::move(logger), mainWindow));
		self->registerFrontendEventCallback();
		return self;
	}

private:
	MainPluginContext(std::shared_ptr<const Logger::ILogger> logger, QMainWindow *mainWindow)
		: logger_(logger ? std::move(logger)
				 : throw std::invalid_argument(
					   "LoggerIsNullError(MainPluginContext::MainPluginContext)")),
		  runtime_(std::make_shared<Scripting::ScriptingRuntime>(logger_)),
		  dock_(new UI::StreamSegmenterDock(runtime_, logger_, mainWindow))
	{
		obs_frontend_add_dock_by_id("live_stream_segmenter_dock", obs_module_text("LiveStreamSegmenterDock"),
					    dock_);

		profileContext_ = std::make_shared<ProfileContext>(logger_, dock_);
	}

	void registerFrontendEventCallback()
	{
		handleFrontendEventWeakSelfPtr_ = new std::weak_ptr<MainPluginContext>(shared_from_this());
		obs_frontend_add_event_callback(handleFrontendEvent, handleFrontendEventWeakSelfPtr_);
	}

	static void handleFrontendEvent(enum obs_frontend_event event, void *private_data) noexcept
	{
		auto *weakSelfPtr = static_cast<std::weak_ptr<MainPluginContext> *>(private_data);

		if (auto self = weakSelfPtr->lock()) {
			std::shared_ptr<Logger::ILogger> logger = self->logger_;

			if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGING) {
				std::scoped_lock lock(self->profileContextMutex_);
				self->profileContext_.reset();
			} else if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
				std::scoped_lock lock(self->profileContextMutex_);
				self->profileContext_ = std::make_shared<ProfileContext>(logger, self->dock_);
				logger->info("ProfileChanged");
			}
		}
	}

	const std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	UI::StreamSegmenterDock *const dock_;

	std::shared_ptr<ProfileContext> profileContext_ = nullptr;
	mutable std::mutex profileContextMutex_;
	std::weak_ptr<MainPluginContext> *handleFrontendEventWeakSelfPtr_ = nullptr;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
