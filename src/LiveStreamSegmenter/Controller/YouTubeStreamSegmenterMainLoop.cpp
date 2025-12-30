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

#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

#include <QMessageBox>

#include <quickjs.h>

#include <CurlVectorWriter.hpp>
#include <EventScriptingContext.hpp>
#include <GoogleAuthManager.hpp>
#include <Join.hpp>
#include <nlohmann/json.hpp>
#include <ScriptingDatabase.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeTypes.hpp>
#include <ResumeOnQThreadPool.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<const Logger::ILogger> logger,
	QWidget *parent)
	: QObject(nullptr),
	  runtime_(std::move(runtime)),
	  authStore_(std::move(authStore)),
	  eventHandlerStore_(std::move(eventHandlerStore)),
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
	mainLoopTask_ = mainLoop(channel_, runtime_, authStore_, eventHandlerStore_, logger_, parent_);
	mainLoopTask_.start();
	logger_->info("YouTubeStreamSegmenterMainLoopStarted");
}

void YouTubeStreamSegmenterMainLoop::startContinuousSession()
{
	logger_->info("StartContinuousYouTubeSession");
	channel_.send(Message{MessageType::StartContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::stopContinuousSession()
{
	logger_->info("StopContinuousYouTubeSession");
	channel_.send(Message{MessageType::StopContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::segmentCurrentSession()
{
	logger_->info("SegmentCurrentYouTubeSession");
	channel_.send(Message{MessageType::SegmentCurrentSession});
}

namespace {

Async::Task<void> startContinuousSessionTask(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
					     std::shared_ptr<Store::AuthStore> authStore,
					     std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
					     std::shared_ptr<const Logger::ILogger> logger)
{
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	Scripting::ScriptingDatabase database(runtime, ctx, logger, eventHandlerStore->getEventHandlerDatabasePath(),
					      true);
	context.setupContext();
	database.setupContext();
	context.setupLocalStorage();

	const std::string scriptContent = eventHandlerStore->getEventHandlerScript();
	context.loadEventHandler(scriptContent.c_str());

	std::string result = context.executeFunction("onCreateYouTubeLiveBroadcast", R"({})");
	nlohmann::json j = nlohmann::json::parse(result);
	YouTubeApi::YouTubeLiveBroadcastSettings settings = j.template get<YouTubeApi::YouTubeLiveBroadcastSettings>();

	std::string accessToken;
	GoogleAuth::GoogleAuthManager authManager(authStore->getGoogleOAuth2ClientCredentials(), logger);
	GoogleAuth::GoogleTokenState tokenState = authStore->getGoogleTokenState();
	if (tokenState.isAuthorized()) {
		if (tokenState.isAccessTokenFresh()) {
			logger->info("YouTubeAccessTokenFresh");
			accessToken = tokenState.access_token;
		} else {
			logger->info("YouTubeAccessTokenNotFresh");
			GoogleAuth::GoogleAuthResponse freshAuthResponse =
				authManager.fetchFreshAuthResponse(tokenState.refresh_token);
			GoogleAuth::GoogleTokenState newTokenState =
				tokenState.withUpdatedAuthResponse(freshAuthResponse);
			authStore->setGoogleTokenState(newTokenState);
			accessToken = freshAuthResponse.access_token;
			logger->info("YouTubeAccessTokenFetched");
		}
	}

	if (accessToken.empty()) {
		logger->error("YouTubeAccessTokenUnavailable");
		throw std::runtime_error(
			"YouTubeAccessTokenUnavailable(YouTubeStreamSegmenterMainLoop::startContinuousSessionTask)");
	}

	YouTubeApi::YouTubeApiClient apiClient(logger);

	YouTubeApi::YouTubeLiveBroadcast liveBroadcast = apiClient.createLiveBroadcast(accessToken, settings);

	nlohmann::json setThumbnailEventObj{
		{"LiveBroadcast", liveBroadcast},
	};
	std::string setThumbnailEventObjJson = setThumbnailEventObj.dump();
	std::string thumbnailResult = context.executeFunction("onSetThumbnailOnCreatedYouTubeLiveBroadcast",
							      setThumbnailEventObjJson.c_str());
	nlohmann::json jThumbnail = nlohmann::json::parse(thumbnailResult);

	if (jThumbnail.contains("videoId") && jThumbnail["videoId"].is_string()) {
		auto videoId = jThumbnail.at("videoId").get<std::string>();

		if (jThumbnail.contains("thumbnailFile") && jThumbnail["thumbnailFile"].is_string()) {
			auto thumbnailFile = jThumbnail.at("thumbnailFile").get<std::string>();
			std::filesystem::path thumbnailPath(thumbnailFile);
			apiClient.setThumbnail(accessToken, videoId, thumbnailPath);
			logger->info("YouTubeThumbnailSet", {{"videoId", videoId}, {"thumbnailFile", thumbnailFile}});
		} else {
			logger->warn("ThumbnailFileMissing", {{"videoId", videoId}});
		}
	} else {
		logger->warn("SkippingThumbnailSetDueToMissingVideoId");
	}

	co_return;
}

} // anonymous namespace

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(Async::Channel<Message> &channel,
							   std::shared_ptr<Scripting::ScriptingRuntime> runtime,
							   std::shared_ptr<Store::AuthStore> authStore,
							   std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
							   std::shared_ptr<const Logger::ILogger> logger,
							   QWidget *parent)
{
	while (true) {
		std::optional<Message> message = co_await channel.receive();

		if (!message.has_value()) {
			break;
		}

		co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

		try {
			switch (message->type) {
			case MessageType::StartContinuousSession: {
				auto task = startContinuousSessionTask(runtime, authStore, eventHandlerStore, logger);
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
				logger->warn("UnknownMessageType");
			}
		} catch (const std::exception &e) {
			logger->error("MainLoopError", {{"exception", e.what()}});
		} catch (...) {
			logger->error("MainLoopUnknownError");
		}
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
