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

#include "YouTubeStore.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

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
	std::scoped_lock lock(mutex_);
	logger_ = std::move(logger);
}

void YouTubeStore::save() const
{
	// Stub
}

void YouTubeStore::restore()
{
	// Stub
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
