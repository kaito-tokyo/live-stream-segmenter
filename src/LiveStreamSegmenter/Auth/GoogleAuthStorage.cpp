/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "GoogleAuthStorage.hpp"

#include <filesystem>
#include <fstream>

#include <obs-module.h>

#include <ObsUnique.hpp>

namespace fs = std::filesystem;

using json = nlohmann::json;

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

namespace {

fs::path getFilePath()
{
	return fs::u8path(unique_obs_module_config_path("GoogleAuthStorage.json").get());
}

} // namespace

std::optional<GoogleAuthState> GoogleAuthStorage::load()
{
	fs::path path = getFilePath();

	std::ifstream file(path);
	if (!file.is_open()) {
		return std::nullopt;
	}

	try {
		json j;
		file >> j;

		GoogleAuthState authState;
		authState.refreshToken = j.value("refresh_token", "");
		authState.email = j.value("email", "");
		authState.accessToken = j.value("access_token", "");
		authState.scope = j.value("scope", "");

		if (j.contains("expires_at")) {
			int64_t expiresAt = j["expires_at"].get<int64_t>();
			authState.tokenExpiration =
				std::chrono::system_clock::time_point(std::chrono::seconds(expiresAt));
		} else {
			authState.tokenExpiration = std::chrono::system_clock::time_point();
		}

		return authState;
	} catch (...) {
		return std::nullopt;
	}
}

void GoogleAuthStorage::save(const GoogleAuthState &authState)
{
	fs::path path = getFilePath();

	std::ofstream file(path);
	if (file.is_open()) {
		json j{{"refresh_token", authState.refreshToken},
		       {"email", authState.email},
		       {"access_token", authState.accessToken},
		       {"scope", authState.scope}};

		if (authState.tokenExpiration.time_since_epoch().count() != 0) {
			auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
						 authState.tokenExpiration.time_since_epoch())
						 .count();
			j["expires_at"] = expiresAt;
		}

		file << j.dump(4);
	}
}

void GoogleAuthStorage::clear()
{
	fs::remove(getFilePath());
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
