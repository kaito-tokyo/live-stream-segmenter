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

#include <optional>

#include <QMessageBox>

#include <Join.hpp>
#include <YouTubeApiClient.hpp>
#include <GoogleAuthManager.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(std::shared_ptr<Store::AuthStore> authStore,
							       std::shared_ptr<const Logger::ILogger> logger,
							       QWidget *parent)
	: QObject(nullptr),
	  authStore_(std::move(authStore)),
	  logger_(std::move(logger)),
	  parent_(parent)
{
}

YouTubeStreamSegmenterMainLoop::~YouTubeStreamSegmenterMainLoop()
{
	channel_.close();
	Async::join(std::move(mainLoopTask_));
}

void YouTubeStreamSegmenterMainLoop::startMainLoop()
{
	mainLoopTask_ = mainLoop(channel_, authStore_, logger_, parent_);
	mainLoopTask_.start();
	logger_->info("YouTubeStreamSegmenterMainLoop started");
}

void YouTubeStreamSegmenterMainLoop::startContinuousSession()
{
	logger_->info("Starting continuous YouTube live stream session");
	channel_.send(Message{MessageType::StartContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::stopContinuousSession()
{
	logger_->info("Stopping continuous YouTube live stream session");
	channel_.send(Message{MessageType::StopContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::segmentCurrentSession()
{
	logger_->info("Segmenting current YouTube live stream session");
	channel_.send(Message{MessageType::SegmentCurrentSession});
}

namespace {

Async::Task<void> startContinuousSessionTask(std::shared_ptr<Store::AuthStore> authStore,
					     std::shared_ptr<const Logger::ILogger> logger)
{
	GoogleAuth::GoogleAuthManager authManager(authStore->getGoogleOAuth2ClientCredentials(), logger);
	GoogleAuth::GoogleTokenState tokenState = authStore->getGoogleTokenState();
	std::string accessToken;
	if (tokenState.isAuthorized()) {
		if (tokenState.isAccessTokenFresh()) {
			logger->info("A fresh access token for YouTube is available.");
			accessToken = tokenState.access_token;
		} else {
			logger->info(
				"Access token for YouTube is not fresh. Fetching a fresh access token using the refresh token.");
			GoogleAuth::GoogleAuthResponse freshAuthResponse =
				authManager.fetchFreshAuthResponse(tokenState.refresh_token);
			GoogleAuth::GoogleTokenState newTokenState =
				tokenState.withUpdatedAuthResponse(freshAuthResponse);
			authStore->setGoogleTokenState(newTokenState);
			accessToken = freshAuthResponse.access_token;
			logger->info("Fetched a fresh access token for YouTube.");
		}
	}
	YouTubeApi::YouTubeApiClient apiClient(logger);
	apiClient.createLiveStream(accessToken, "TEST", "Test Stream from Live Stream Segmenter", "hls");
	co_return;
}

} // anonymous namespace

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(Async::Channel<Message> &channel,
							   std::shared_ptr<Store::AuthStore> authStore,
							   std::shared_ptr<const Logger::ILogger> logger,
							   QWidget *parent)
{
	while (true) {
		std::optional<Message> message = co_await channel.receive();

		if (!message.has_value()) {
			break;
		}

		switch (message->type) {
		case MessageType::StartContinuousSession: {
			auto task = startContinuousSessionTask(authStore, logger);
			task.start();
			co_await task;
			break;
		}
		case MessageType::StopContinuousSession:
			QMessageBox::information(parent, "Info", "StopContinuousSession received");
			break;
		case MessageType::SegmentCurrentSession:
			QMessageBox::information(parent, "Info", "SegmentCurrentSession received");
			break;
		default:
			logger->warn("Received unknown message type in YouTubeStreamSegmenterMainLoop");
		}
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
