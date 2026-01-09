/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Stream Segmenter - Controller Module
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

#include <array>

#include <QObject>
#include <QThread>
#include <QTimer>

#include <QCoro/QCoroTask>

#include <CurlHelper/CurlHandle.hpp>
#include <Logger/ILogger.hpp>
#include <Scripting/ScriptingRuntime.hpp>
#include <Store/AuthStore.hpp>
#include <Store/EventHandlerStore.hpp>
#include <Store/YouTubeStore.hpp>
#include <YouTubeApi/YouTubeApiClient.hpp>

#include "YouTubeStreamSegmenterWorker.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenterController : public QObject {
	Q_OBJECT
public:
	YouTubeStreamSegmenterController(std::shared_ptr<const Logger::ILogger> logger,
					 std::shared_ptr<CurlHelper::CurlHandle> curl,
					 std::shared_ptr<YouTubeApi::YouTubeApiClient> youtubeApiClient,
					 std::shared_ptr<Scripting::ScriptingRuntime> runtime,
					 std::shared_ptr<Store::AuthStore> authStore,
					 std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
					 std::shared_ptr<Store::YouTubeStore> youtubeStore, QObject *parent = nullptr);

	~YouTubeStreamSegmenterController() noexcept;

	void start();
	void stop();

signals:
	void requestStartSession();
	void requestStopSession();
	void requestSegmentSession();

	void tick(int remainingTime);

private slots:
	void onTick();
	void onSegmentTimeout();

private:
	std::shared_ptr<const Logger::ILogger> logger_;

	QThread workerThread_;
	YouTubeStreamSegmenterWorker *worker_ = nullptr;

private:
	QTimer *tickTimer_;
	QTimer *segmentTimer_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
