/*
 * Live Stream Segmenter - Store Module
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

#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <ILogger.hpp>
#include <NullLogger.hpp>
#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class EventHandlerStore {
public:
	EventHandlerStore() = default;
	~EventHandlerStore() noexcept = default;

	EventHandlerStore(const EventHandlerStore &) = delete;
	EventHandlerStore &operator=(const EventHandlerStore &) = delete;
	EventHandlerStore(EventHandlerStore &&) = delete;
	EventHandlerStore &operator=(EventHandlerStore &&) = delete;

	void setLogger(std::shared_ptr<const Logger::ILogger> logger) { logger_ = std::move(logger); }

	void setEventHandlerScript(std::string eventHandlerScript)
	{
		std::scoped_lock lock(mutex_);
		eventHandlerScript_ = std::move(eventHandlerScript);
	}

	std::string getEventHandlerScript() const
	{
		std::scoped_lock lock(mutex_);
		return eventHandlerScript_;
	}

	std::filesystem::path getEventHandlerDatabasePath() const
	{
		BridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
		if (!profilePathRaw) {
			logger_->error("ProfilePathError");
			return {};
		}

		std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
		return profilePath / "live-stream-segmenter_EventHandlerStore_db.sqlite";
	}

	bool save() const
	{
		std::shared_ptr<const Logger::ILogger> logger = logger_;

		std::filesystem::path configPath = getConfigPath();
		if (configPath.empty()) {
			return false;
		}

		std::scoped_lock lock(mutex_);

		nlohmann::json j{
			{"eventHandlerScript", eventHandlerScript_},
		};

		std::ofstream ofs(configPath, std::ios::out | std::ios::trunc);
		if (!ofs.is_open()) {
			logger->error("FileOpenError");
			return false;
		}

		ofs << j.dump();

		return true;
	}

	void restore()
	{
		std::shared_ptr<const Logger::ILogger> logger = logger_;

		std::filesystem::path configPath = getConfigPath();
		if (configPath.empty()) {
			logger->info("FileMissing");
			return;
		}

		std::scoped_lock lock(mutex_);
		std::ifstream ifs(configPath, std::ios::in);
		if (!ifs.is_open()) {
			logger->error("FileOpenError");
			return;
		}

		nlohmann::json j;
		ifs >> j;

		try {
			eventHandlerScript_ = j.value("eventHandlerScript", std::string{});
		} catch (...) {
			logger->error("JsonParseError");
			eventHandlerScript_ = {};
			throw;
		}
	}

private:
	std::filesystem::path getConfigPath() const
	{
		BridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
		if (!profilePathRaw) {
			logger_->error("ProfilePathError");
			return {};
		}

		std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
		return profilePath / "live-stream-segmenter_EventHandlerStore.json";
	}

	mutable std::mutex mutex_;
	std::string eventHandlerScript_;

	std::shared_ptr<const Logger::ILogger> logger_{Logger::NullLogger::instance()};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
