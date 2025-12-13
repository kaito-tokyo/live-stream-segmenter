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

#include <mutex>

#include <obs.h>
#include <util/config-file.h>

#include <ILogger.hpp>
#include <ObsUnique.hpp>

#include <GoogleAuthManager.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Service {

class AuthService {
public:
    AuthService() = default;
    ~AuthService() noexcept = default;

    AuthService(const AuthService &) = delete;
    AuthService &operator=(const AuthService &) = delete;
    AuthService(AuthService &&) = delete;
    AuthService &operator=(AuthService &&) = delete;

    void setGoogleCredential(std::string clientId, std::string clientSecret) {
        std::lock_guard<std::mutex> lock(mutex_);

        googleClientId_ = std::move(clientId);
        googleClientSecret_ = std::move(clientSecret);

        googleAuthManager_ = std::make_shared<Auth::GoogleAuthManager>(
            googleClientId_, googleClientSecret_, logger_);
    }

    void loadGoogleCredential() {
        config_t *config = obs_frontend_get_profile_config();
        const char *googleClientId = config_get_string(config, PLUGIN_NAME, "googleClientId");
        const char *googleClientSecret = config_get_string(config, PLUGIN_NAME, "googleClientSecret");
        if (googleClientId && googleClientSecret) {
            setGoogleCredential(googleClientId, googleClientSecret);
        }
    }

    void saveGoogleCredential() {
        std::lock_guard<std::mutex> lock(mutex_);
        config_t *config = obs_frontend_get_profile_config();
        config_set_string(config, PLUGIN_NAME, "googleClientId", googleClientId_.c_str());
        config_set_string(config, PLUGIN_NAME, "googleClientSecret", googleClientSecret_.c_str());
    }

    std::shared_ptr<Auth::GoogleAuthManager> getGoogleAuthManager() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return googleAuthManager_;
    }

private:
    std::shared_ptr<Logger::ILogger> logger_;

    std::string googleClientId_;
    std::string googleClientSecret_;

    mutable std::mutex mutex_;
    std::shared_ptr<Auth::GoogleAuthManager> googleAuthManager_ = nullptr;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
