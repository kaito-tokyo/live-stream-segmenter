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

#include <QCoro/QCoroTask>

#include <CurlHelper/CurlHandle.hpp>
#include <Jthread/Jthread.hpp>
#include <Logger/ILogger.hpp>
#include <Scripting/ScriptingRuntime.hpp>
#include <Store/AuthStore.hpp>
#include <Store/EventHandlerStore.hpp>
#include <Store/YouTubeStore.hpp>
#include <YouTubeApi/YouTubeApiClient.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenterWorker : public QObject {
	Q_OBJECT
public:
	YouTubeStreamSegmenterWorker(QObject *mainContext, QThread *workerThread,
				     std::shared_ptr<const Logger::ILogger> logger,
				     std::shared_ptr<CurlHelper::CurlHandle> curl,
				     std::shared_ptr<YouTubeApi::YouTubeApiClient> youtubeApiClient,
				     std::shared_ptr<Scripting::ScriptingRuntime> runtime,
				     std::shared_ptr<Store::AuthStore> authStore,
				     std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
				     std::shared_ptr<Store::YouTubeStore> youtubeStore);

	~YouTubeStreamSegmenterWorker() noexcept;

public slots:
	QCoro::Task<> onStartSession();
	QCoro::Task<> onStopSession();
	QCoro::Task<> onSegmentSession();

signals:
	void sessionStarted();
	void sessionStopped();
	void errorOccurred(QString message);

private:
	QObject *mainContext_;
	QThread *workerThread_;
	std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<CurlHelper::CurlHandle> curl_;
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient_;
	std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	std::shared_ptr<Store::AuthStore> authStore_;
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	std::shared_ptr<Store::YouTubeStore> youtubeStore_;

	int currentLiveStreamIndex_ = 0;
	std::array<std::shared_ptr<YouTTubeApi::YouTubeLiveBroadcast>, 2> liveBroadcasts_;
	Jthread::stop_source stopSource_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
