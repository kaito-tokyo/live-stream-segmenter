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

#include <obs-frontend-api.h>

#include <CurlVectorWriter.hpp>
#include <EventScriptingContext.hpp>
#include <GoogleAuthManager.hpp>
#include <Join.hpp>
#include <nlohmann/json.hpp>
#include <ObsUnique.hpp>
#include <ResumeOnQObject.hpp>
#include <ResumeOnQThreadPool.hpp>
#include <ResumeOnQTimerSingleShot.hpp>
#include <ScriptingDatabase.hpp>
#include <YouTubeTypes.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

namespace {

std::string getAccessToken(std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<const Logger::ILogger> logger)
{
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

	return accessToken;
}

} // anonymous namespace

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
	: QObject(nullptr),
	  youTubeApiClient_(std::move(youTubeApiClient)),
	  runtime_(std::move(runtime)),
	  authStore_(std::move(authStore)),
	  eventHandlerStore_(std::move(eventHandlerStore)),
	  youtubeStore_(std::move(youtubeStore)),
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
	mainLoopTask_ = mainLoop(channel_, youTubeApiClient_, runtime_, authStore_, eventHandlerStore_, youtubeStore_,
				 logger_, parent_);
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
	channel_.send(Message{MessageType::SegmentLiveBroadcast});
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(
	Async::Channel<Message> &channel, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
{
	int currentLiveStreamIndex = 0;

	while (true) {
		std::optional<Message> message = co_await channel.receive();

		if (!message.has_value()) {
			break;
		}

		co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

		Async::Task<void> task;
		try {
			switch (message->type) {
			case MessageType::StartContinuousSession: {
				Async::Task<void> task = startContinuousSessionTask(channel);
				co_await task;
				break;
			}
			case MessageType::StopContinuousSession: {
				Async::Task<void> task = stopContinuousSessionTask(channel);
				co_await task;
				break;
			}
			case MessageType::SegmentLiveBroadcast: {
				YouTubeApi::YouTubeLiveStream nextLiveStream;
				if (currentLiveStreamIndex == 0) {
					nextLiveStream = youtubeStore->getStreamKeyB();
				} else if (currentLiveStreamIndex == 1) {
					nextLiveStream = youtubeStore->getStreamKeyA();
				}
				Async::Task<void> task = segmentLiveBroadcastTask(channel, logger, youTubeApiClient,
										  runtime, authStore, eventHandlerStore,
										  youtubeStore, parent, nextLiveStream);
				co_await task;
				currentLiveStreamIndex = 1 - currentLiveStreamIndex;
				break;
			}
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

Async::Task<void> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(Async::Channel<Message> &channel)
{
	channel.send({MessageType::SegmentLiveBroadcast});
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
}

Async::Task<void>
YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask([[maybe_unused]] Async::Channel<Message> &channel)
{
	obs_frontend_streaming_stop();
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::segmentLiveBroadcastTask(
	[[maybe_unused]] Async::Channel<Message> &channel, std::shared_ptr<const Logger::ILogger> logger,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	QWidget *parent, const YouTubeApi::YouTubeLiveStream &nextLiveStream)
{
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

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

	std::string accessToken = getAccessToken(authStore, logger);

	YouTubeApi::YouTubeLiveBroadcast liveBroadcast = youTubeApiClient->createLiveBroadcast(accessToken, settings);

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
			youTubeApiClient->setThumbnail(accessToken, videoId, thumbnailPath);
			logger->info("YouTubeThumbnailSet", {{"videoId", videoId}, {"thumbnailFile", thumbnailFile}});
		} else {
			logger->warn("ThumbnailFileMissing", {{"videoId", videoId}});
		}

		logger->info("YouTubeLiveBroadcastCreated", {{"broadcastId", liveBroadcast.id}});

		logger->info("WaitingForLiveBroadcastCreated");
		co_await AsyncQt::ResumeOnQTimerSingleShot{1000};

		youTubeApiClient->bindLiveBroadcast(accessToken, liveBroadcast.id, nextLiveStream.id);
		logger->info("YouTubeLiveBroadcastBound");
	} else {
		logger->warn("SkippingThumbnailSetDueToMissingVideoId");
	}

	logger->info("WaitingForLiveBroadcastBound");
	co_await AsyncQt::ResumeOnQTimerSingleShot{1000};

	co_await AsyncQt::ResumeOnQObject{parent};

	if (nextLiveStream.cdn.ingestionType == "rtmp") {
		logger->info("CreatingYouTubeRTMPService");

		auto settings = BridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "YouTube - RTMP");

		obs_data_set_string(settings.get(), "server", "rtmps://a.rtmps.youtube.com:443/live2");

		obs_data_set_string(settings.get(), "key", nextLiveStream.cdn.ingestionInfo.streamName.c_str());
		obs_service_t *service =
			obs_service_create("rtmp_common", "YouTube RTMP Service", settings.get(), NULL);

		obs_frontend_set_streaming_service(service);
	} else if (nextLiveStream.cdn.ingestionType == "hls") {
		logger->info("CreatingYouTubeHLSService");

		auto settings = BridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "YouTube - HLS");

		obs_data_set_string(
			settings.get(), "server",
			"https://a.upload.youtube.com/http_upload_hls?cid={stream_key}&copy=0&file=out.m3u8");

		obs_data_set_string(settings.get(), "key", nextLiveStream.cdn.ingestionInfo.streamName.c_str());
		obs_service_t *service = obs_service_create("rtmp_common", "YouTube HLS Service", settings.get(), NULL);

		obs_frontend_set_streaming_service(service);
	} else {
		logger->error("UnsupportedIngestionType", {{"type", nextLiveStream.cdn.ingestionType}});
		co_return;
	}

	obs_frontend_streaming_start();

	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

	logger->info("WaitingForLiveBroadcastStart");
	co_await AsyncQt::ResumeOnQTimerSingleShot(5000);

	YouTubeApi::YouTubeLiveBroadcast startedLiveBroadcast = youTubeApiClient->transitionLiveBroadcast(accessToken, liveBroadcast.id, "live");
	logger->info("YouTubeLiveBroadcastStarted");
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
