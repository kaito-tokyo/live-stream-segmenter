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

#include <obs-frontend-api.h>

#include <ILogger.hpp>

#include <AuthService.hpp>
#include <StreamSegmenterDock.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class ProfileContext {
public:
	ProfileContext(std::shared_ptr<const Logger::ILogger> logger, UI::StreamSegmenterDock *dock)
		: logger_(std::move(logger)),
		  dock_(dock),
		  authService_(std::make_shared<Service::AuthService>(logger_))
	{
		authService_->restoreGoogleOAuth2ClientCredential();
	}

	~ProfileContext() noexcept = default;

	ProfileContext(const ProfileContext &) = delete;
	ProfileContext &operator=(const ProfileContext &) = delete;
	ProfileContext(ProfileContext &&) = delete;
	ProfileContext &operator=(ProfileContext &&) = delete;

private:
	const std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<Service::AuthService> authService_;
	UI::StreamSegmenterDock *const dock_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
