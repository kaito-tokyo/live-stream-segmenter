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

#define PLUGIN_NAME "live-backgroundremoval-lite"

#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static std::shared_ptr<const Logger::ILogger> g_logger;
static std::shared_ptr<Controller::MainPluginContext> g_mainPluginContext;

bool obs_module_load(void)
{
	g_logger = std::make_shared<BridgeUtils::ObsLogger>("[" PLUGIN_NAME "]");

	if (QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window())) {
		g_mainPluginContext = Controller::MainPluginContext::create(g_logger, mainWindow);
	} else {
		blog(LOG_INFO, "[" PLUGIN_NAME "] plugin load failed (version " PLUGIN_VERSION ")");
		return false;
	}

	blog(LOG_INFO, "[" PLUGIN_NAME "] plugin loaded successfully (version " PLUGIN_VERSION ")");
	return true;
}

void obs_module_unload(void)
{
	g_mainPluginContext.reset();
	g_logger.reset();
	blog(LOG_INFO, "[" PLUGIN_NAME "] plugin unloaded");
}
