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

#pragma once

#include <string>
#include <optional>

#include "GoogleAuthState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleAuthStorage {
public:
	GoogleAuthStorage() = default;
	virtual ~GoogleAuthStorage() = default;

	virtual std::optional<GoogleAuthState> load();
	virtual void save(const GoogleAuthState &state);
	virtual void clear();
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
