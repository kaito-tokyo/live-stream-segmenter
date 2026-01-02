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

#include "YouTubeStreamSegmenterMainLoop.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

#include <QMessageBox>
#include <QObject>

#include <quickjs.h>

#include <obs-frontend-api.h>

#include <CurlWriteCallback.hpp>
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

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
	: QObject(nullptr),
	  runtime_(runtime ? std::move(runtime)
			   : throw std::invalid_argument("RuntimeIsNullError(YouTubeStreamSegmenterMainLoop)")),
	  authStore_(authStore ? std::move(authStore)
			       : throw std::invalid_argument("AuthStoreIsNullError(YouTubeStreamSegmenterMainLoop)")),
	  eventHandlerStore_(eventHandlerStore
				     ? std::move(eventHandlerStore)
				     : throw std::invalid_argument(
					       "EventHandlerStoreIsNullError(YouTubeStreamSegmenterMainLoop)")),
	  youtubeStore_(youtubeStore ? std::move(youtubeStore)
				     : throw std::invalid_argument(
					       "YouTubeStoreIsNullError(YouTubeStreamSegmenterMainLoop)")),
	  logger_(logger ? std::move(logger)
			 : throw std::invalid_argument("LoggerIsNullError(YouTubeStreamSegmenterMainLoop)")),
	  parent_(parent),
	  curl_(std::make_shared<CurlHelper::CurlHandle>()),
	  youTubeApiClient_(std::make_shared<YouTubeApi::YouTubeApiClient>(curl_))
{
	youTubeApiClient_->setLogger(logger_);
}

YouTubeStreamSegmenterMainLoop::~YouTubeStreamSegmenterMainLoop()
{
	channel_.close();
	Async::join(std::move(mainLoopTask_));
}

void YouTubeStreamSegmenterMainLoop::startMainLoop()
{
	mainLoopTask_ = mainLoop(channel_, curl_, youTubeApiClient_, runtime_, authStore_, eventHandlerStore_,
				 youtubeStore_, logger_, parent_);
	mainLoopTask_.start();
	logger_->info("YouTubeStreamSegmenterMainLoopStarted");
}

void YouTubeStreamSegmenterMainLoop::onStartContinuousSession()
{
	channel_.send(Message{MessageType::StartContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onStopContinuousSession()
{
	channel_.send(Message{MessageType::StopContinuousSession});
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(
	Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
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
				Async::Task<void> task = startContinuousSessionTask(
					curl, youTubeApiClient, runtime, authStore, eventHandlerStore, youtubeStore,
					currentLiveStreamIndex, parent, logger);
				co_await task;
				break;
			}
			case MessageType::StopContinuousSession: {
				Async::Task<void> task = stopContinuousSessionTask(channel, curl, youTubeApiClient,
										   authStore, youtubeStore, logger);
				co_await task;
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

namespace {

class TaskBoundLogger : public Logger::ILogger {
public:
	TaskBoundLogger(std::shared_ptr<const Logger::ILogger> baseLogger, std::string_view taskName)
		: baseLogger_(std::move(baseLogger)),
		  taskName_(taskName)
	{
	}

private:
	void log(Logger::LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const Logger::LogField> context) const noexcept override
	{
		std::vector<Logger::LogField> extendedContext;
		extendedContext.reserve(context.size() + 1);
		extendedContext.emplace_back("taskName", taskName_);
		extendedContext.insert(extendedContext.end(), context.begin(), context.end());
		baseLogger_->log(level, name, loc, std::span<const Logger::LogField>(extendedContext));
	}

	std::shared_ptr<const Logger::ILogger> baseLogger_;
	std::string taskName_;
};

// Must be called from a worker thread and returns on a worker thread
std::string getAccessToken(std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<Store::AuthStore> authStore,
			   std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeAccessTokenGetting");

	std::string accessToken;

	GoogleAuth::GoogleOAuth2ClientCredentials clientCredentials = authStore->getGoogleOAuth2ClientCredentials();
	GoogleAuth::GoogleAuthManager authManager(curl, clientCredentials, logger);

	GoogleAuth::GoogleTokenState tokenState = authStore->getGoogleTokenState();

	if (!tokenState.isAuthorized()) {
		logger->error("YouTubeAccessTokenNotAuthorized");
		throw std::runtime_error(
			"YouTubeAccessTokenNotAuthorized(YouTubeStreamSegmenterMainLoop::getAccessToken)");
	}

	if (tokenState.isAccessTokenFresh()) {
		logger->info("YouTubeAccessTokenFresh");
		accessToken = tokenState.access_token;
	} else {
		logger->info("YouTubeAccessTokenRefreshing");

		GoogleAuth::GoogleAuthResponse freshAuthResponse =
			authManager.fetchFreshAuthResponse(tokenState.refresh_token);

		tokenState.loadAuthResponse(freshAuthResponse);

		authStore->setGoogleTokenState(tokenState);

		authStore->save();

		accessToken = freshAuthResponse.access_token;

		logger->info("YouTubeAccessTokenRefreshed");
	}

	logger->info("YouTubeAccessTokenGotten");

	return accessToken;
}

// Must be called from the main thread and returns on the main thread
Async::Task<void> ensureOBSStreamingStopped(std::shared_ptr<const Logger::ILogger> logger)
{
	if (!obs_frontend_streaming_active()) {
		logger->info("OBSStreamingAlreadyStopped");
		co_return;
	}

	logger->info("OBSStreamingStopping");

	struct StopStreamingAwaiter {
		std::coroutine_handle<> h_;

		static void callback(enum obs_frontend_event event, void *data)
		{
			if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
				auto *self = static_cast<StopStreamingAwaiter *>(data);
				if (self->h_) {
					self->h_.resume();
				}
			}
		}

		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> h) noexcept
		{
			h_ = h;
			obs_frontend_add_event_callback(callback, this);
			obs_frontend_streaming_stop();
		}

		void await_resume() noexcept { obs_frontend_remove_event_callback(callback, this); }
	};

	co_await StopStreamingAwaiter{};

	logger->info("OBSStreamingStopped");
}

// Must be called from a worker thread and returns on a worker thread
void completeActiveLiveBroadcasts(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				  std::string accessToken, std::span<std::string> liveStreamIds,
				  std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCompletingAllActive");

	std::vector<YouTubeApi::YouTubeLiveBroadcast> activeBroadcasts =
		youTubeApiClient->listLiveBroadcastsByStatus(accessToken, "active");

	for (const YouTubeApi::YouTubeLiveBroadcast &broadcast : activeBroadcasts) {
		const std::optional<std::string> &boundStreamId = broadcast.contentDetails.boundStreamId;

		if (!boundStreamId.has_value())
			continue;

		auto it = std::ranges::find(liveStreamIds, boundStreamId.value());
		if (it == liveStreamIds.end())
			continue;

		logger->info("YouTubeLiveBroadcastCompleting",
			     {{"broadcastId", broadcast.id}, {"title", broadcast.snippet.title}});

		youTubeApiClient->transitionLiveBroadcast(accessToken, broadcast.id, "complete");

		logger->info("YouTubeLiveBroadcastCompleted",
			     {{"broadcastId", broadcast.id}, {"title", broadcast.snippet.title}});
	}

	logger->info("YouTubeLiveBroadcastCompletedAllActive");
}

// Must be called from a worker thread and returns on a worker thread
YouTubeApi::YouTubeLiveBroadcast createLiveBroadcast(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
						     std::string accessToken,
						     std::shared_ptr<Scripting::EventScriptingContext> context,
						     std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCreating");

	std::string result = context->executeFunction("onCreateYouTubeLiveBroadcast", R"({})");
	nlohmann::json j = nlohmann::json::parse(result);
	YouTubeApi::YouTubeLiveBroadcastSettings settings;
	j.get_to(settings);

	logger->info("YouTubeLiveBroadcastInserting");

	YouTubeApi::YouTubeLiveBroadcast liveBroadcast = youTubeApiClient->insertLiveBroadcast(accessToken, settings);

	logger->info("YouTubeLiveBroadcastInserted",
		     {{"broadcastId", liveBroadcast.id}, {"title", liveBroadcast.snippet.title}});

	nlohmann::json setThumbnailEventObj{
		{"LiveBroadcast", liveBroadcast},
	};
	std::string setThumbnailEventObjJson = setThumbnailEventObj.dump();
	std::string thumbnailResult = context->executeFunction("onSetThumbnailAfterYouTubeLiveBroadcastCreated",
							       setThumbnailEventObjJson.c_str());
	nlohmann::json jThumbnail = nlohmann::json::parse(thumbnailResult);

	if (jThumbnail.contains("videoId") && jThumbnail["videoId"].is_string()) {
		std::string videoId;
		jThumbnail.at("videoId").get_to(videoId);

		if (jThumbnail.contains("thumbnailFile") && jThumbnail["thumbnailFile"].is_string()) {
			std::string thumbnailFile;
			jThumbnail.at("thumbnailFile").get_to(thumbnailFile);

			std::filesystem::path thumbnailPath(reinterpret_cast<const char8_t *>(thumbnailFile.data()));

			logger->info("YouTubeLiveBroadcastThumbnailSetting",
				     {{"videoId", videoId}, {"thumbnailFile", thumbnailFile}});

			youTubeApiClient->setThumbnail(accessToken, videoId, thumbnailPath);

			logger->info("YouTubeLiveBroadcastThumbnailSet",
				     {{"videoId", videoId}, {"thumbnailFile", thumbnailFile}});
		} else {
			logger->warn("YouTubeLiveBroadcastThumbnailFileMissing", {{"videoId", videoId}});
		}
	} else {
		logger->warn("YouTubeLiveBroadcastThumbnailVideoIdMissing");
	}

	logger->info("YouTubeLiveBroadcastCreated");

	return liveBroadcast;
}

// Must be called from a worker thread and returns on the main thread
Async::Task<void> startStreaming(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				 std::string accessToken, QObject *parent,
				 std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast> nextLiveBroadcast,
				 std::shared_ptr<YouTubeApi::YouTubeLiveStream> nextLiveStream,
				 std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("StreamingStarting");

	logger->info("YouTubeLiveBroadcastBindingLiveStream",
		     {{"broadcastId", nextLiveBroadcast->id}, {"streamId", nextLiveStream->id}});

	youTubeApiClient->bindLiveBroadcast(accessToken, nextLiveBroadcast->id, nextLiveStream->id);

	logger->info("YouTubeLiveBroadcastBoundToLiveStream",
		     {{"broadcastId", nextLiveBroadcast->id}, {"streamId", nextLiveStream->id}});

	if (nextLiveStream->cdn.ingestionType == "rtmp") {
		logger->info("OBSStreamingYouTubeRTMPServiceCreating");

		auto settings = ObsBridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "YouTube - RTMP");
		obs_data_set_string(settings.get(), "server", "rtmps://a.rtmps.youtube.com:443/live2");
		obs_data_set_string(settings.get(), "key", nextLiveStream->cdn.ingestionInfo.streamName.c_str());

		obs_service_t *service =
			obs_service_create("rtmp_common", "YouTube RTMP Service", settings.get(), NULL);

		obs_frontend_set_streaming_service(service);
		obs_service_release(service);

		logger->info("OBSStreamingYouTubeRTMPServiceCreated");
	} else if (nextLiveStream->cdn.ingestionType == "hls") {
		logger->info("OBSStreamingYouTubeHLSServiceCreating");

		auto settings = ObsBridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "YouTube - HLS");
		obs_data_set_string(
			settings.get(), "server",
			"https://a.upload.youtube.com/http_upload_hls?cid={stream_key}&copy=0&file=out.m3u8");
		obs_data_set_string(settings.get(), "key", nextLiveStream->cdn.ingestionInfo.streamName.c_str());

		obs_service_t *service = obs_service_create("rtmp_common", "YouTube HLS Service", settings.get(), NULL);

		obs_frontend_set_streaming_service(service);
		obs_service_release(service);

		logger->info("OBSStreamingYouTubeHLSServiceCreated");
	} else {
		logger->error("OBSStreamingUnsupportedYouTubeIngestionTypeError",
			      {{"ingestionType", nextLiveStream->cdn.ingestionType}});
		throw std::runtime_error(
			"OBSStreamingUnsupportedYouTubeIngestionTypeError(YouTubeStreamSegmenterMainLoop::startOBSStreaming)");
	}

	obs_frontend_streaming_start();

	logger->info("OBSStreamingStarted");

	logger->info("YouTubeLiveStreamWaitingForActive", {{"liveStreamId", nextLiveStream->id}});

	std::array<std::string_view, 1> nextLiveStreamIdArray{nextLiveStream->id};
	for (int maxAttempts = 20; true; --maxAttempts) {
		co_await AsyncQt::ResumeOnQTimerSingleShot{5000, parent};
		co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

		std::string maxAttemptsStr = std::to_string(maxAttempts);
		logger->info("YouTubeLiveStreamCheckingIfActive",
			     {{"liveStreamId", nextLiveStream->id}, {"attemptsLeft", maxAttemptsStr}});

		std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
			youTubeApiClient->listLiveStreams(accessToken, nextLiveStreamIdArray);

		if (liveStreams.size() == 1 && liveStreams[0].status.has_value() &&
		    liveStreams[0].status->streamStatus == "active") {
			logger->info("YouTubeLiveStreamActive", {{"liveStreamId", nextLiveStream->id}});
			break;
		}

		if (maxAttempts <= 0) {
			logger->error("YouTubeLiveStreamTimeout", {{"liveStreamId", nextLiveStream->id}});
			co_return;
		}
	}

	logger->info("YouTubeLiveBroadcastTransitioningToTesting",
		     {{"broadcastId", nextLiveBroadcast->id}, {"title", nextLiveBroadcast->snippet.title}});

	youTubeApiClient->transitionLiveBroadcast(accessToken, nextLiveBroadcast->id, "testing");

	logger->info("YouTubeLiveBroadcastTransitionedToTesting",
		     {{"broadcastId", nextLiveBroadcast->id}, {"title", nextLiveBroadcast->snippet.title}});

	co_await AsyncQt::ResumeOnQTimerSingleShot{5000, parent};
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

	logger->info("YouTubeLiveBroadcastTransitioningToLive",
		     {{"broadcastId", nextLiveBroadcast->id}, {"title", nextLiveBroadcast->snippet.title}});

	youTubeApiClient->transitionLiveBroadcast(accessToken, nextLiveBroadcast->id, "live");

	logger->info("YouTubeLiveBroadcastTransitionedToLive",
		     {{"broadcastId", nextLiveBroadcast->id}, {"title", nextLiveBroadcast->snippet.title}});
}

} // anonymous namespace

Async::Task<void> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, QObject *parent, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	// on the main thread
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::startContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionStarting");

	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(logger);

	logger->info("OBSStreamingEnsuredStopped");

	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
	// on a worker thread

	// --- Scripting ---
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	std::shared_ptr<Scripting::EventScriptingContext> context =
		std::make_shared<Scripting::EventScriptingContext>(runtime, ctx, logger);
	Scripting::ScriptingDatabase database(runtime, ctx, logger, eventHandlerStore->getEventHandlerDatabasePath(),
					      true);
	context->setupContext();
	database.setupContext();
	context->setupLocalStorage();

	const std::string scriptContent = eventHandlerStore->getEventHandlerScript();
	context->loadEventHandler(scriptContent.c_str());

	// --- YouTube access token ---
	std::string accessToken = getAccessToken(curl, authStore, logger);

	// --- Complete active broadcasts ---
	logger->info("YouTubeLiveBroadcastCompletingActive");

	std::string currentLiveStreamId = youtubeStore->getLiveStreamId(currentLiveStreamIndex);
	std::string nextLiveStreamId = youtubeStore->getLiveStreamId(1 - currentLiveStreamIndex);
	if (currentLiveStreamId.empty() || nextLiveStreamId.empty()) {
		logger->error("YouTubeLiveStreamIdNotSet");
		throw std::runtime_error(
			"YouTubeLiveStreamIdNotSet(YouTubeStreamSegmenterMainLoop::startContinuousSessionTask)");
	}

	std::array<std::string, 2> liveStreamIds{
		currentLiveStreamId,
		nextLiveStreamId,
	};

	completeActiveLiveBroadcasts(youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Create an initial live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingInitial");

	auto initialLiveBroadcast = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(
		createLiveBroadcast(youTubeApiClient, accessToken, context, logger));

	logger->info("YouTubeLiveBroadcastCreatedInitial",
		     {{"broadcastId", initialLiveBroadcast->id}, {"title", initialLiveBroadcast->snippet.title}});

	// --- Create the next live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingNext");

	YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast =
		createLiveBroadcast(youTubeApiClient, accessToken, context, logger);

	logger->info("YouTubeLiveBroadcastCreatedNext",
		     {{"broadcastId", nextLiveBroadcast.id}, {"title", nextLiveBroadcast.snippet.title}});

	// --- Get the next live stream ---
	logger->info("YouTubeLiveStreamGettingNext", {{"liveStreamId", nextLiveStreamId}});

	std::array<std::string_view, 1> nextLiveStreamIdArray{nextLiveStreamId};
	std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
		youTubeApiClient->listLiveStreams(accessToken, nextLiveStreamIdArray);
	if (liveStreams.empty()) {
		logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", nextLiveStreamId}});
		throw std::runtime_error(
			"YouTubeLiveStreamNotFound(YouTubeStreamSegmenterMainLoop::startContinuousSessionTask)");
	} else if (liveStreams.size() > 1) {
		logger->warn("YouTubeLiveStreamMultipleFound", {{"liveStreamId", nextLiveStreamId}});
	}
	auto nextLiveStream = std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);

	logger->info("YouTubeLiveStreamGottenNext", {{"liveStreamId", nextLiveStream->id}});

	// --- Start streaming the initial live broadcast ---
	logger->info("StreamingStarting");

	co_await startStreaming(youTubeApiClient, accessToken, parent, initialLiveBroadcast, nextLiveStream, logger);

	logger->info("StreamingStarted");

	logger->info("ContinuousYouTubeSessionStarted");
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask(
	[[maybe_unused]] Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::YouTubeStore> youtubeStore, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	// on the main thread
	const std::shared_ptr<const Logger::ILogger> logger =
		std::make_shared<TaskBoundLogger>(baseLogger, "StopContinuousYouTubeSessionTask");

	logger->info("ContinuousYouTubeSessionStopping");

	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(logger);

	logger->info("OBSStreamingEnsuredStopped");

	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
	// on a worker thread

	// --- YouTube access token ---
	std::string accessToken = getAccessToken(curl, authStore, logger);

	// --- Complete active broadcasts ---
	logger->info("YouTubeLiveBroadcastCompletingActive");

	std::array<std::string, 2> liveStreamIds{
		youtubeStore->getLiveStreamId(0),
		youtubeStore->getLiveStreamId(1),
	};
	if (liveStreamIds[0].empty() || liveStreamIds[1].empty()) {
		logger->error("YouTubeLiveStreamIdNotSet");
		throw std::runtime_error(
			"YouTubeLiveStreamIdNotSet(YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask)");
	}

	completeActiveLiveBroadcasts(youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	logger->info("ContinuousYouTubeSessionStopped");
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
