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
		return j.get<GoogleAuthState>();
	} catch (...) {
		return std::nullopt;
	}
}

void GoogleAuthStorage::save(const GoogleAuthState &authState)
{
	fs::path path = getFilePath();

	std::ofstream file(path);
	if (file.is_open()) {
        json j = authState;
		file << j.dump();
	}
}

void GoogleAuthStorage::clear()
{
	fs::remove(getFilePath());
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
