#include "TokenStorage.hpp"
#include <fstream>
#include <obs-module.h> // obs_module_get_config_path

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

std::optional<GoogleAuthState> TokenStorage::load()
{
	std::string path = getFilePath();
	if (path.empty())
		return std::nullopt;

	std::ifstream file(path);
	if (!file.is_open()) {
		return std::nullopt;
	}

	try {
		json j;
		file >> j;
		return GoogleAuthState::fromJson(j);
	} catch (...) {
		return std::nullopt;
	}
}

void TokenStorage::save(const GoogleAuthState &state)
{
	std::string path = getFilePath();
	if (path.empty())
		return;

	std::ofstream file(path);
	if (file.is_open()) {
		file << state.toJson().dump(4);
	}
}

void TokenStorage::clear()
{
	save(GoogleAuthState());
}

std::string TokenStorage::getFilePath() const
{
	char *path = obs_module_get_config_path(obs_current_module(), "token.json");
	if (!path)
		return "";

	std::string sPath(path);
	bfree(path);
	return sPath;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
