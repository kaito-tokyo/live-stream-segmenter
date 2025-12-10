#pragma once

#include <string>
#include <optional>
#include "GoogleAuthState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class TokenStorage {
public:
	TokenStorage() = default;
	virtual ~TokenStorage() = default;

	virtual std::optional<GoogleAuthState> load();
	virtual void save(const GoogleAuthState &state);
	virtual void clear();

private:
	std::string getFilePath() const;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
