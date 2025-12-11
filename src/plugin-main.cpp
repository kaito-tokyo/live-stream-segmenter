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

#include <memory>

#include <QMainWindow>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <ILogger.hpp>
#include <ObsLogger.hpp>

#include <StreamSegmenterDock.hpp>

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::Logger;

using namespace KaitoTokyo::LiveStreamSegmenter::UI;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static std::shared_ptr<ILogger> g_logger;

// This pointer will be freed automatically by OBS on unload
static StreamSegmenterDock *g_dock_ = nullptr;

bool obs_module_load(void)
{
	g_logger = std::make_shared<ObsLogger>("[" PLUGIN_NAME "]");

	auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	if (!mainWindow) {
		blog(LOG_ERROR, "[" PLUGIN_NAME "] Failed to get main window");
		return false;
	}

	g_dock_ = new StreamSegmenterDock(g_logger, mainWindow);
	obs_frontend_add_dock_by_id("live_stream_segmenter_dock", obs_module_text("LiveStreamSegmenterDock"), g_dock_);

	blog(LOG_INFO, "[" PLUGIN_NAME "] plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[" PLUGIN_NAME "] plugin unloaded");
}
