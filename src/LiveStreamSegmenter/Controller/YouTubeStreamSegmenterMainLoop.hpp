/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
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

#include <condition_variable>
#include <memory>

#include <QObject>
#include <QWidget>

#include <AuthStore.hpp>
#include <Channel.hpp>
#include <CurlHandle.hpp>
#include <EventHandlerStore.hpp>
#include <ILogger.hpp>
#include <ScriptingRuntime.hpp>
#include <Task.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeStore.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenterMainLoop : public QObject {
	Q_OBJECT

	enum class MessageType {
		StartContinuousSession,
		StopContinuousSession,
		SegmentLiveBroadcast,
	};

	struct Message {
		MessageType type;
	};

public:
	YouTubeStreamSegmenterMainLoop(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
				       std::shared_ptr<Store::AuthStore> authStore,
				       std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
				       std::shared_ptr<Store::YouTubeStore> youtubeStore,
				       std::shared_ptr<const Logger::ILogger> logger, QWidget *parent);

	~YouTubeStreamSegmenterMainLoop() override;

	YouTubeStreamSegmenterMainLoop(const YouTubeStreamSegmenterMainLoop &) = delete;
	YouTubeStreamSegmenterMainLoop &operator=(const YouTubeStreamSegmenterMainLoop &) = delete;
	YouTubeStreamSegmenterMainLoop(YouTubeStreamSegmenterMainLoop &&) = delete;
	YouTubeStreamSegmenterMainLoop &operator=(YouTubeStreamSegmenterMainLoop &&) = delete;

	void startMainLoop();

public slots:
	void startContinuousSession();
	void stopContinuousSession();
	void segmentCurrentSession();

private:
	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	const std::shared_ptr<Store::AuthStore> authStore_;
	const std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	const std::shared_ptr<Store::YouTubeStore> youtubeStore_;
	const std::shared_ptr<const Logger::ILogger> logger_;
	QWidget *const parent_;

	const std::shared_ptr<CurlHelper::CurlHandle> curl_;
	const std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient_;

	Async::Channel<Message> channel_;
	Async::Task<void> mainLoopTask_;

	static Async::Task<void> mainLoop(Async::Channel<Message> &channel,
					  std::shared_ptr<CurlHelper::CurlHandle> curl,
					  std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
					  std::shared_ptr<Scripting::ScriptingRuntime> runtime,
					  std::shared_ptr<Store::AuthStore> authStore,
					  std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
					  std::shared_ptr<Store::YouTubeStore> youtubeStore,
					  std::shared_ptr<const Logger::ILogger> logger, QWidget *parent);

	static Async::Task<void> startContinuousSessionTask(Async::Channel<Message> &channel);

	static Async::Task<void> stopContinuousSessionTask(Async::Channel<Message> &channel);

	static Async::Task<void>
	segmentLiveBroadcastTask(Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
				 std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				 std::shared_ptr<Scripting::ScriptingRuntime> runtime,
				 std::shared_ptr<Store::AuthStore> authStore,
				 std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
				 std::shared_ptr<const Logger::ILogger> logger, QWidget *parent,
				 const YouTubeApi::YouTubeLiveStream &currentLiveStream,
				 const YouTubeApi::YouTubeLiveStream &nextLiveStream);
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
