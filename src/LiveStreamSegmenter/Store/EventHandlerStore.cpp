/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Stream Segmenter - Store Module
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

#include "EventHandlerStore.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

EventHandlerStore::EventHandlerStore() = default;

EventHandlerStore::~EventHandlerStore() noexcept = default;

std::filesystem::path EventHandlerStore::getConfigPath()
{
	ObsBridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
	if (!profilePathRaw) {
		throw std::runtime_error("GetCurrentProfilePathFailed(getConfigPath)");
	}

	std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
	return profilePath / "live-stream-segmenter_EventHandlerStore.json";
}

void EventHandlerStore::setLogger(std::shared_ptr<const Logger::ILogger> logger)
{
	std::scoped_lock lock(mutex_);
	logger_ = std::move(logger);
}

void EventHandlerStore::setEventHandlerScript(std::string eventHandlerScript)
{
	std::scoped_lock lock(mutex_);
	eventHandlerScript_ = std::move(eventHandlerScript);
}

std::string EventHandlerStore::getEventHandlerScript() const
{
	std::scoped_lock lock(mutex_);
	return eventHandlerScript_;
}

std::filesystem::path EventHandlerStore::getEventHandlerDatabasePath() const
{
	ObsBridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
	if (!profilePathRaw) {
		logger_->error("ProfilePathError(getEventHandlerDatabasePath)");
		throw std::runtime_error("ProfilePathError(getEventHandlerDatabasePath)");
	}

	std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
	return profilePath / "live-stream-segmenter_EventHandlerStore_db.sqlite";
}

void EventHandlerStore::save() const
{
	const std::filesystem::path configPath = getConfigPath();

	std::string eventHandlerScript;
	{
		std::scoped_lock lock(mutex_);
		eventHandlerScript = eventHandlerScript_;
	}

	nlohmann::json j{
		{"eventHandlerScript", eventHandlerScript},
	};

	std::filesystem::path tmpConfigPath = configPath;
	tmpConfigPath += ".tmp";
	std::ofstream ofs(tmpConfigPath, std::ios::out);
	if (!ofs.is_open()) {
		logger_->error("FileOpenError", {{"path", configPath.string()}});
		throw std::runtime_error("FileOpenError(save)");
	}

	ofs << j;

	ofs.close();

	std::filesystem::path bakConfigPath = configPath;
	bakConfigPath += ".bak";

	if (std::filesystem::is_regular_file(configPath)) {
		std::filesystem::rename(configPath, bakConfigPath);
	}
	std::filesystem::rename(tmpConfigPath, configPath);
}

void EventHandlerStore::restore()
{
	const std::filesystem::path configPath = getConfigPath();
	if (!std::filesystem::is_regular_file(configPath)) {
		logger_->info("EventHandlerStoreConfigFileNotExist", {{"path", configPath.string()}});
		return;
	}

	std::ifstream ifs(configPath, std::ios::in);
	if (!ifs.is_open()) {
		logger_->error("FileOpenError", {{"path", configPath.string()}});
		throw std::runtime_error("FileOpenError(restore)");
	}

	nlohmann::json j;
	ifs >> j;

	std::scoped_lock lock(mutex_);
	try {
		if (j.contains("eventHandlerScript")) {
			j.at("eventHandlerScript").get_to(eventHandlerScript_);
			logger_->info("RestoredEventHandlerScript");
		}
	} catch (...) {
		eventHandlerScript_.clear();
		throw;
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
