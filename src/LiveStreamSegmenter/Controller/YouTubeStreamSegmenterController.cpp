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

#include "YouTubeStreamSegmenterController.hpp"

#include "YouTubeStreamSegmenterWorker.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenterController::YouTubeStreamSegmenterController(
	std::shared_ptr<const Logger::ILogger> logger, std::shared_ptr<Scripting::ScriptingRuntime> runtime,
	std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
	std::shared_ptr<Store::YouTubeStore> youtubeStore, QObject *parent)
	: QObject(parent),
	  logger_(logger)
{
	auto curl = std::make_shared<CurlHelper::CurlHandle>();
	auto youtubeApiClient = std::make_shared<YouTubeApi::YouTubeApiClient>(curl);

	worker_ = new YouTubeStreamSegmenterWorker(parent, &workerThread_, logger, curl, youtubeApiClient, runtime,
						   authStore, eventHandlerStore, youtubeStore);
	worker_->moveToThread(&workerThread_);

	connect(this, &YouTubeStreamSegmenterController::requestStartSession, worker_,
		&YouTubeStreamSegmenterWorker::onStartSession, Qt::QueuedConnection);
	connect(this, &YouTubeStreamSegmenterController::requestStopSession, worker_,
		&YouTubeStreamSegmenterWorker::onStopSession, Qt::QueuedConnection);
	connect(this, &YouTubeStreamSegmenterController::requestSegmentSession, worker_,
		&YouTubeStreamSegmenterWorker::onSegmentSession, Qt::QueuedConnection);

	connect(worker_, &YouTubeStreamSegmenterWorker::sessionStarted, this,
		&YouTubeStreamSegmenterController::sessionStarted, Qt::QueuedConnection);
	connect(worker_, &YouTubeStreamSegmenterWorker::sessionStopped, this,
		&YouTubeStreamSegmenterController::sessionStopped, Qt::QueuedConnection);
	connect(worker_, &YouTubeStreamSegmenterWorker::sessionSegmented, this,
		&YouTubeStreamSegmenterController::sessionSegmented, Qt::QueuedConnection);
	connect(worker_, &YouTubeStreamSegmenterWorker::errorOccurred, this,
		&YouTubeStreamSegmenterController::errorOccurred, Qt::QueuedConnection);

	connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

	workerThread_.start();

	tickTimer_ = new QTimer(this);
	segmentTimer_ = new QTimer(this);
	tickTimer_->setTimerType(Qt::VeryCoarseTimer);
	segmentTimer_->setTimerType(Qt::VeryCoarseTimer);

	connect(tickTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterController::onTick);
	connect(segmentTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterController::onSegmentTimeout);
}

YouTubeStreamSegmenterController::~YouTubeStreamSegmenterController() noexcept
{
	workerThread_.quit();
	workerThread_.wait();
}

void YouTubeStreamSegmenterController::start()
{
	tickTimer_->start(1000);
	segmentTimer_->start(3600 * 1000);

	emit requestStartSession();
}

void YouTubeStreamSegmenterController::stop()
{
	tickTimer_->stop();
	segmentTimer_->stop();

	emit requestStopSession();
}

void YouTubeStreamSegmenterController::segmentNow()
{
	emit requestSegmentSession();
}

void YouTubeStreamSegmenterController::onTick()
{
	emit tick(segmentTimer_->remainingTime());
}

void YouTubeStreamSegmenterController::onSegmentTimeout()
{
	emit requestSegmentSession();
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
