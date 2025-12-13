/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file 
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#include <memory>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <ILogger.hpp>
#include <ObsLogger.hpp>

#include <MainPluginContext.hpp>

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::Logger;

using namespace KaitoTokyo::LiveStreamSegmenter::Controller;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static std::shared_ptr<const ILogger> g_logger;
static std::unique_ptr<MainPluginContext> g_mainPluginContext;

bool obs_module_load(void)
{
	g_logger = std::make_shared<ObsLogger>("[" PLUGIN_NAME "]");

	QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!mainWindow) {
		g_logger->error("Failed to get main window");
		return false;
	}

	g_mainPluginContext = std::make_unique<MainPluginContext>(g_logger, mainWindow);

	g_logger->info("plugin loaded successfully (version {})", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	g_logger->info("plugin unloaded");
}
