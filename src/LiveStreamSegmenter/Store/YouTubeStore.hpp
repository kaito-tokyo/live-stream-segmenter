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

#pragma once

#include <filesystem>
#include <memory>
#include <mutex>

#include <ILogger.hpp>
#include <YouTubeTypes.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class YouTubeStore {
public:
	YouTubeStore();

	~YouTubeStore() noexcept;

	YouTubeStore(const YouTubeStore &) = delete;
	YouTubeStore &operator=(const YouTubeStore &) = delete;
	YouTubeStore(YouTubeStore &&) = delete;
	YouTubeStore &operator=(YouTubeStore &&) = delete;

	static std::filesystem::path getConfigPath();

	void setLogger(std::shared_ptr<const Logger::ILogger> logger);

	void setStreamKeyA(const YouTubeApi::YouTubeLiveStream &streamKey);

	void setStreamKeyB(const YouTubeApi::YouTubeLiveStream &streamKey);

	YouTubeApi::YouTubeLiveStream getStreamKeyA() const;

	YouTubeApi::YouTubeLiveStream getStreamKeyB() const;

	void save() const;

	void restore();

private:
	mutable std::mutex mutex_;
	YouTubeApi::YouTubeLiveStream streamKeyA_;
	YouTubeApi::YouTubeLiveStream streamKeyB_;

	std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
