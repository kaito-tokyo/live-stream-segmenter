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
#include <StreamSegmenterDock.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class ProfileContext {
public:
	ProfileContext(std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
		: logger_(std::move(logger)),
		  dock_(dock),
		  authStore_(std::make_shared<Store::AuthStore>(logger_))
	{
		authStore_->restoreAuthStore();
		dock_->setAuthStore(authStore_);
	}

	~ProfileContext() noexcept = default;

	ProfileContext(const ProfileContext &) = delete;
	ProfileContext &operator=(const ProfileContext &) = delete;
	ProfileContext(ProfileContext &&) = delete;
	ProfileContext &operator=(ProfileContext &&) = delete;

private:
	const std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<Store::AuthStore> authStore_;
	UI::StreamSegmenterDock *const dock_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
