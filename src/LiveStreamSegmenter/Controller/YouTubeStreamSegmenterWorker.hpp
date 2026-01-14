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

#include <KaitoTokyo/CurlHelper/CurlHandle.hpp>
#include <KaitoTokyo/Jthread/Jthread.hpp>
#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/YouTubeApi/YouTubeApiClient.hpp>

#include <AuthStore.hpp>
#include <EventHandlerStore.hpp>
#include <ScriptingRuntime.hpp>
#include <YouTubeStore.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenterWorker : public QObject {
	Q_OBJECT
public:
	struct CurrentLiveBroadcast {
		std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast> liveBroadcast;
		std::shared_ptr<YouTubeApi::YouTubeLiveStream> liveStream;
	};

	YouTubeStreamSegmenterWorker(QObject *mainContext, QThread *workerThread,
				     std::shared_ptr<const Logger::ILogger> logger,
				     std::shared_ptr<CurlHelper::CurlHandle> curl,
				     std::shared_ptr<YouTubeApi::YouTubeApiClient> youtubeApiClient,
				     std::shared_ptr<Scripting::ScriptingRuntime> runtime,
				     std::shared_ptr<Store::AuthStore> authStore,
				     std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
				     std::shared_ptr<Store::YouTubeStore> youtubeStore);

	~YouTubeStreamSegmenterWorker() noexcept;

	void cancelCurrentTask() { stopSource_.request_stop(); }

public slots:
	QCoro::Task<> onStartSession();
	QCoro::Task<> onStopSession();
	QCoro::Task<> onSegmentSession();

signals:
	void sessionStarted();
	void sessionStopped();
	void sessionSegmented();
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

	std::optional<CurrentLiveBroadcast> currentLiveBroadcast_;
	std::array<std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>, 2> liveBroadcasts_;
	Jthread::stop_source stopSource_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
