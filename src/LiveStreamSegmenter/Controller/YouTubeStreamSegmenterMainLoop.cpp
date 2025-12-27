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
#include <QThreadPool>

#include <ResumeOnQThreadPool.hpp>
#include <ResumeOnQtMainThread.hpp>
#include <TaskQtLauncher.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(std::shared_ptr<const Logger::ILogger> logger)
	: QObject(nullptr),
	  logger_(std::move(logger))
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

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(YouTubeStreamSegmenterMainLoop *self)
{
	struct RunGuard {
		std::atomic<bool> &flag;
		RunGuard(std::atomic<bool> &f) : flag(f) { flag.store(true, std::memory_order_release); }
		~RunGuard() { flag.store(false, std::memory_order_release); }
	} runGuard{self->isRunning_};

	std::shared_ptr<const Logger::ILogger> logger = self->logger_;

	while (true) {
		co_await ResumeOnQThreadPool(QThreadPool::globalInstance());

		Async::Task<void> task;
		{
			std::unique_lock lock(self->mutex_);

			while (self->taskQueue_.empty() && !self->stopRequested_) {
				self->cv_.wait(lock);
			}

			if (self->stopRequested_) {
				break;
			}

			task = std::move(self->taskQueue_.front());
			self->taskQueue_.pop_front();
		}

		try {
			co_await task;
		} catch (const std::exception &e) {
			logger->error("error=Error\tlocation=YouTubeStreamSegmenterMainLoop::mainLoop\tmessage={}",
				      e.what());
		} catch (...) {
			logger->error("error=UnknownError\tlocation=YouTubeStreamSegmenterMainLoop::mainLoop");
		}
	}
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(YouTubeStreamSegmenterMainLoop *)
{
	co_await ResumeOnQtMainThread();
	QMessageBox::information(nullptr, "Info", "Starting continuous YouTube live stream session");
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask(YouTubeStreamSegmenterMainLoop *)
{
	co_await ResumeOnQtMainThread();
	QMessageBox::information(nullptr, "Info", "Stopping continuous YouTube live stream session");
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::segmentCurrentSessionTask(YouTubeStreamSegmenterMainLoop *)
{
	co_await ResumeOnQtMainThread();
	QMessageBox::information(nullptr, "Info", "Segmenting current YouTube live stream session");
}

void YouTubeStreamSegmenterMainLoop::enqueueTask(Async::Task<void> task)
{
	{
		std::scoped_lock lock(mutex_);
		taskQueue_.emplace_back(std::move(task));
	}
	cv_.notify_one();
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
