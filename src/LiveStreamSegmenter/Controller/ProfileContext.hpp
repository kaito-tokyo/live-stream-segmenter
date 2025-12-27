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
#include <YouTubeStore.hpp>
#include <StreamSegmenterDock.hpp>

#include "YouTubeStreamSegmenter.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class ProfileContext {
public:
	ProfileContext(std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
		: logger_(std::move(logger)),
		  authStore_(std::make_shared<Store::AuthStore>(logger_)),
		  youTubeStore_(std::make_shared<Store::YouTubeStore>(logger_)),
		  dock_(dock),
		  youTubeStreamSegmenter_(std::make_shared<YouTubeStreamSegmenter>(logger_))
	{
		authStore_->restoreAuthStore();
		youTubeStore_->restoreYouTubeStore();
		dock_->setAuthStore(authStore_);
		dock_->setYouTubeStore(youTubeStore_);

		std::shared_ptr<YouTubeStreamSegmenter> youTubeStreamSegmenter = youTubeStreamSegmenter_;

		QObject::connect(dock_, &UI::StreamSegmenterDock::startButtonClicked, [youTubeStreamSegmenter]() {
			youTubeStreamSegmenter->startContinuousSession();
		});

		QObject::connect(dock_, &UI::StreamSegmenterDock::stopButtonClicked, [youTubeStreamSegmenter]() {
			youTubeStreamSegmenter->stopContinuousSession();
		});

		QObject::connect(dock_, &UI::StreamSegmenterDock::segmentNowButtonClicked, [youTubeStreamSegmenter]() {
			youTubeStreamSegmenter->segmentCurrentSession();
		});
	}

	~ProfileContext() noexcept = default;

	ProfileContext(const ProfileContext &) = delete;
	ProfileContext &operator=(const ProfileContext &) = delete;
	ProfileContext(ProfileContext &&) = delete;
	ProfileContext &operator=(ProfileContext &&) = delete;

private:
	const std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<Store::AuthStore> authStore_;
	std::shared_ptr<Store::YouTubeStore> youTubeStore_;
	UI::StreamSegmenterDock *const dock_;
	std::shared_ptr<YouTubeStreamSegmenter> youTubeStreamSegmenter_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
