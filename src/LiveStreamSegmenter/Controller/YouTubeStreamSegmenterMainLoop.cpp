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

#include "YouTubeStreamSegmenterMainLoop.hpp"

#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>
#include <QMainWindow>

#include <ResumeOnQObject.hpp>
#include <ResumeOnQThreadPool.hpp>
#include <Task.hpp>
#include <Generator.hpp>

#include <obs-frontend-api.h>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(std::shared_ptr<const Logger::ILogger> logger,
							       QWidget *parent)
	: QObject(nullptr),
	  logger_(std::move(logger)),
	  parent_(parent)
{
}

YouTubeStreamSegmenterMainLoop::~YouTubeStreamSegmenterMainLoop()
{
	{
		std::scoped_lock lock(mutex_);
		stopRequested_ = true;
		cv_.notify_all();
	}

	if (mainLoopTask_) {
		while (isRunning_.load(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}
}

void YouTubeStreamSegmenterMainLoop::startMainLoop()
{
	mainLoopTask_ = mainLoop(this);
	mainLoopTask_.start();
}

void YouTubeStreamSegmenterMainLoop::startContinuousSession()
{
	logger_->info("Starting continuous YouTube live stream session");
	enqueueTask(startContinuousSessionTask(this));
}

void YouTubeStreamSegmenterMainLoop::stopContinuousSession()
{
	logger_->info("Stopping continuous YouTube live stream session");
	enqueueTask(stopContinuousSessionTask(this));
}

void YouTubeStreamSegmenterMainLoop::segmentCurrentSession()
{
	logger_->info("Segmenting current YouTube live stream session");
	enqueueTask(segmentCurrentSessionTask(this));
}

Async::Generator<void> YouTubeStreamSegmenterMainLoop::mainLoop(YouTubeStreamSegmenterMainLoop *self)
{
	while (true) {
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
