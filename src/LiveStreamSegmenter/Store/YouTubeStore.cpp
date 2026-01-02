/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
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

#include "YouTubeStore.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

YouTubeStore::YouTubeStore() = default;

YouTubeStore::~YouTubeStore() noexcept = default;

std::filesystem::path YouTubeStore::getConfigPath()
{
	ObsBridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
	if (!profilePathRaw) {
		return {};
	}

	std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
	return profilePath / "live-stream-segmenter_YouTubeStore.json";
}

void YouTubeStore::setLogger(std::shared_ptr<const Logger::ILogger> logger)
{
	logger_ = std::move(logger);
}

void YouTubeStore::setStreamKeyA(const YouTubeApi::YouTubeLiveStream &streamKey)
{
	std::scoped_lock lock(mutex_);
	streamKeyA_ = streamKey;
}

void YouTubeStore::setStreamKeyB(const YouTubeApi::YouTubeLiveStream &streamKey)
{
	std::scoped_lock lock(mutex_);
	streamKeyB_ = streamKey;
}

YouTubeApi::YouTubeLiveStream YouTubeStore::getStreamKeyA() const
{
	std::scoped_lock lock(mutex_);
	return streamKeyA_;
}

YouTubeApi::YouTubeLiveStream YouTubeStore::getStreamKeyB() const
{
	std::scoped_lock lock(mutex_);
	return streamKeyB_;
}

void YouTubeStore::save() const
{
	const std::filesystem::path configPath = getConfigPath();

	YouTubeApi::YouTubeLiveStream streamKeyA;
	YouTubeApi::YouTubeLiveStream streamKeyB;
	{
		std::scoped_lock lock(mutex_);
		streamKeyA = streamKeyA_;
		streamKeyB = streamKeyB_;
	}

	nlohmann::json j{
		{"streamKeyA", streamKeyA},
		{"streamKeyB", streamKeyB},
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

void YouTubeStore::restore()
{
	const std::filesystem::path configPath = getConfigPath();
	if (!std::filesystem::is_regular_file(configPath)) {
		logger_->info("YouTubeStoreConfigFileNotExist", {{"path", configPath.string()}});
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
		if (j.contains("streamKeyA")) {
			j.at("streamKeyA").get_to(streamKeyA_);
			logger_->info("RestoredStreamKeyA");
		}
		if (j.contains("streamKeyB")) {
			j.at("streamKeyB").get_to(streamKeyB_);
			logger_->info("RestoredStreamKeyB");
		}
	} catch (...) {
		streamKeyA_ = {};
		streamKeyB_ = {};
		throw;
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
