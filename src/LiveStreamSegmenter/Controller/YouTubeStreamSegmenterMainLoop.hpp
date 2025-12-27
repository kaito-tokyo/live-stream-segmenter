/*
 * Live Stream Segmenter - Controller Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include <Channel.hpp>
#include <ILogger.hpp>
#include <Task.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenterMainLoop : public QObject {
	Q_OBJECT

	enum class MessageType {
		StartContinuousSession,
		StopContinuousSession,
		SegmentCurrentSession,
	};

	struct Message {
		MessageType type;
	};

public:
	explicit YouTubeStreamSegmenterMainLoop(std::shared_ptr<const Logger::ILogger> logger, QWidget *parent);
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
	std::shared_ptr<const Logger::ILogger> logger_;
	QWidget *const parent_;

	Async::Channel<Message> channel_;
	Async::Task<void> mainLoopTask_;

	static Async::Task<void> mainLoop(Async::Channel<Message> &channel,
					  std::shared_ptr<const Logger::ILogger> logger, QWidget *parent);
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
