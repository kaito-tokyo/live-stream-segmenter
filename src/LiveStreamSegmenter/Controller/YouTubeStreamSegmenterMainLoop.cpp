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
#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <KaitoTokyo/Async/Join.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQObject.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQThreadPool.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQTimerSingleShot.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>
#include <KaitoTokyo/GoogleAuth/GoogleAuthManager.hpp>
#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>
#include <KaitoTokyo/YouTubeApi/YouTubeTypes.hpp>

#include <EventScriptingContext.hpp>
#include <ScriptingDatabase.hpp>

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
	  youTubeApiClient_(std::make_shared<YouTubeApi::YouTubeApiClient>(curl_)),
	  tickTimer_(new QTimer(this)),
	  segmentTimer_(new QTimer(this))
{
	youTubeApiClient_->setLogger(logger_);

	tickTimer_->setTimerType(Qt::VeryCoarseTimer);
	segmentTimer_->setTimerType(Qt::VeryCoarseTimer);

	connect(tickTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterMainLoop::onTick);
	connect(segmentTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterMainLoop::onSegmentTimeout);
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

void YouTubeStreamSegmenterMainLoop::startTickTimer(int interval)
{
	tickTimer_->start(interval);
}

void YouTubeStreamSegmenterMainLoop::startSegmentTimer(int interval)
{
	segmentTimer_->start(interval);
}

void YouTubeStreamSegmenterMainLoop::stopSegmentTimer()
{
	segmentTimer_->stop();
}

void YouTubeStreamSegmenterMainLoop::onTick()
{
	emit tick(segmentTimer_->remainingTime());
}

void YouTubeStreamSegmenterMainLoop::onSegmentTimeout()
{
	channel_.send(Message{MessageType::SegmentContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onStartContinuousSession()
{
	channel_.send(Message{MessageType::StartContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onStopContinuousSession()
{
	segmentTimer_->stop();
	tickTimer_->stop();
	channel_.send(Message{MessageType::StopContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession()
{
	channel_.send(Message{MessageType::SegmentContinuousSession});
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(
	Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
{
	int currentLiveStreamIndex = 0;
	std::array<YouTubeApi::YouTubeLiveBroadcast, 2> liveBroadcasts;

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
				liveBroadcasts = co_await startContinuousSessionTask(
					curl, youTubeApiClient, runtime, authStore, eventHandlerStore, youtubeStore,
					currentLiveStreamIndex, parent, logger);
				break;
			}
			case MessageType::StopContinuousSession: {
				Async::Task<void> task = stopContinuousSessionTask(channel, curl, youTubeApiClient,
										   authStore, youtubeStore, logger);
				co_await task;
				break;
			}
			case MessageType::SegmentContinuousSession: {
				liveBroadcasts = co_await segmentContinuousSessionTask(
					curl, youTubeApiClient, runtime, authStore, eventHandlerStore, youtubeStore,
					currentLiveStreamIndex, liveBroadcasts[1], parent, logger);
				currentLiveStreamIndex = (currentLiveStreamIndex + 1) % 2;
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
				  const std::string &accessToken, std::span<const std::string> liveStreamIds,
				  std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCompletingAllActive");

	const std::vector<YouTubeApi::YouTubeLiveBroadcast> activeLiveBroadcasts =
		youTubeApiClient->listLiveBroadcastsByStatus(accessToken, "active");

	for (const YouTubeApi::YouTubeLiveBroadcast &liveBroadcast : activeLiveBroadcasts) {
		if (!liveBroadcast.contentDetails || !liveBroadcast.contentDetails->boundStreamId) {
			logger->warn("YouTubeLiveBroadcastBoundStreamIdMissing");
			continue;
		}

		const std::string &boundStreamId = *liveBroadcast.contentDetails->boundStreamId;

		const auto it = std::ranges::find(liveStreamIds, boundStreamId);
		if (it == liveStreamIds.end())
			continue;

		if (!liveBroadcast.id) {
			logger->warn("YouTubeLiveBroadcastIdMissing");
			continue;
		}

		const std::string &liveBroadcastId = *liveBroadcast.id;
		const std::string liveBroadcastTitle = liveBroadcast.snippet && liveBroadcast.snippet->title
							       ? *liveBroadcast.snippet->title
							       : "(TITLE MISSING)";

		logger->info("YouTubeLiveBroadcastCompleting",
			     {{"broadcastId", liveBroadcastId}, {"title", liveBroadcastTitle}});

		youTubeApiClient->transitionLiveBroadcast(accessToken, liveBroadcastId, "complete");
		logger->info("YouTubeLiveBroadcastCompleted",
			     {{"broadcastId", liveBroadcastId}, {"title", liveBroadcastTitle}});
	}

	logger->info("YouTubeLiveBroadcastCompletedAllActive");
}

// Must be called from a worker thread and returns on a worker thread
YouTubeApi::YouTubeLiveBroadcast createLiveBroadcast(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
						     const std::string &accessToken,
						     std::shared_ptr<Scripting::EventScriptingContext> context,
						     const std::string &onCreateLiveBroadcastFunctionName,
						     const std::string &onSetThumbnailFunctionName,
						     std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCreating");

	const std::string result = context->executeFunction(onCreateLiveBroadcastFunctionName.c_str(), R"({})");
	const nlohmann::json j = nlohmann::json::parse(result);
	YouTubeApi::InsertingYouTubeLiveBroadcast insertingLiveBroadcast;
	j.at("YouTubeLiveBroadcast").get_to(insertingLiveBroadcast);

	logger->info("YouTubeLiveBroadcastInserting");

	const YouTubeApi::YouTubeLiveBroadcast liveBroadcast =
		youTubeApiClient->insertLiveBroadcast(accessToken, insertingLiveBroadcast);
	const std::string liveBroadcastId = liveBroadcast.id.value_or("(ID MISSING)");
	const std::string liveBroadcastTitle = (liveBroadcast.snippet && liveBroadcast.snippet->title)
						       ? *liveBroadcast.snippet->title
						       : "(TITLE MISSING)";
	logger->info("YouTubeLiveBroadcastInserted", {{"broadcastId", liveBroadcastId}, {"title", liveBroadcastTitle}});

	const nlohmann::json setThumbnailEventObj{
		{"LiveBroadcast", liveBroadcast},
	};
	const std::string setThumbnailEventObjJson = setThumbnailEventObj.dump();
	const std::string thumbnailResult =
		context->executeFunction(onSetThumbnailFunctionName.c_str(), setThumbnailEventObjJson.c_str());
	const nlohmann::json jThumbnail = nlohmann::json::parse(thumbnailResult);

	if (jThumbnail.contains("videoId") && jThumbnail["videoId"].is_string()) {
		std::string videoId;
		jThumbnail.at("videoId").get_to(videoId);

		if (jThumbnail.contains("thumbnailFile") && jThumbnail["thumbnailFile"].is_string()) {
			std::string thumbnailFile;
			jThumbnail.at("thumbnailFile").get_to(thumbnailFile);

			const std::filesystem::path thumbnailPath(
				reinterpret_cast<const char8_t *>(thumbnailFile.data()));

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
				 const std::string &accessToken, QObject *parent,
				 std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast> nextLiveBroadcast,
				 std::shared_ptr<YouTubeApi::YouTubeLiveStream> nextLiveStream,
				 std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("StreamingStarting");

	if (!nextLiveBroadcast->id) {
		logger->error("YouTubeLiveBroadcastIdMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIdMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}
	logger->info("YouTubeLiveBroadcastBindingLiveStream",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"streamId", nextLiveStream->id}});

	youTubeApiClient->bindLiveBroadcast(accessToken, *nextLiveBroadcast->id, nextLiveStream->id);
	logger->info("YouTubeLiveBroadcastBoundToLiveStream",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"streamId", nextLiveStream->id}});

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

	const std::array<std::string, 1> nextLiveStreamIdArray{nextLiveStream->id};
	for (int maxAttempts = 20; true; --maxAttempts) {
		co_await AsyncQt::ResumeOnQTimerSingleShot{5000, parent};
		co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

		const std::string maxAttemptsStr = std::to_string(maxAttempts);
		logger->info("YouTubeLiveStreamCheckingIfActive",
			     {{"liveStreamId", nextLiveStream->id}, {"attemptsLeft", maxAttemptsStr}});

		const std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
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

	if (!nextLiveBroadcast->id) {
		logger->error("YouTubeLiveBroadcastIdMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIdMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}
	const std::string nextLiveBroadcastTitle = (nextLiveBroadcast->snippet && nextLiveBroadcast->snippet->title)
							   ? *nextLiveBroadcast->snippet->title
							   : "(TITLE MISSING)";
	logger->info("YouTubeLiveBroadcastTransitioningToTesting",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	youTubeApiClient->transitionLiveBroadcast(accessToken, *nextLiveBroadcast->id, "testing");

	logger->info("YouTubeLiveBroadcastTransitionedToTesting",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});
	co_await AsyncQt::ResumeOnQTimerSingleShot{5000, parent};
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

	logger->info("YouTubeLiveBroadcastTransitioningToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	youTubeApiClient->transitionLiveBroadcast(accessToken, *nextLiveBroadcast->id, "live");

	logger->info("YouTubeLiveBroadcastTransitionedToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});
}

} // anonymous namespace

Async::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(
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
	const std::string accessToken = getAccessToken(curl, authStore, logger);

	// --- Complete active broadcasts ---
	logger->info("YouTubeLiveBroadcastCompletingActive");

	const std::string currentLiveStreamId = youtubeStore->getLiveStreamId(currentLiveStreamIndex);
	const std::string nextLiveStreamId = youtubeStore->getLiveStreamId(1 - currentLiveStreamIndex);
	if (currentLiveStreamId.empty() || nextLiveStreamId.empty()) {
		logger->error("YouTubeLiveStreamIdNotSet");
		throw std::runtime_error(
			"YouTubeLiveStreamIdNotSet(YouTubeStreamSegmenterMainLoop::startContinuousSessionTask)");
	}

	const std::array<std::string, 2> liveStreamIds{
		currentLiveStreamId,
		nextLiveStreamId,
	};

	completeActiveLiveBroadcasts(youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Create an initial live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingInitial");

	auto initialLiveBroadcast = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(
		createLiveBroadcast(youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastInitial",
				    "onSetYouTubeThumbnailInitial", logger));

	const std::string initialLiveBroadcastId = initialLiveBroadcast->id.value_or("(ID MISSING)");
	const std::string initialLiveBroadcastTitle =
		(initialLiveBroadcast->snippet && initialLiveBroadcast->snippet->title)
			? *initialLiveBroadcast->snippet->title
			: "(TITLE MISSING)";
	logger->info("YouTubeLiveBroadcastCreatedInitial",
		     {{"broadcastId", initialLiveBroadcastId}, {"title", initialLiveBroadcastTitle}});

	// --- Create the next live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingNext");

	const YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast =
		createLiveBroadcast(youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastInitialNext",
				    "onSetYouTubeThumbnailInitialNext", logger);

	const std::string nextLiveBroadcastId = nextLiveBroadcast.id.value_or("(ID MISSING)");
	const std::string nextLiveBroadcastTitle = (nextLiveBroadcast.snippet && nextLiveBroadcast.snippet->title)
							   ? *nextLiveBroadcast.snippet->title
							   : "(TITLE MISSING)";

	logger->info("YouTubeLiveBroadcastCreatedNext",
		     {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

	// --- Get the next live stream ---
	logger->info("YouTubeLiveStreamGettingCurrent", {{"liveStreamId", currentLiveStreamId}});

	const std::array<std::string, 1> currentLiveStreamIdArray{currentLiveStreamId};
	std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
		youTubeApiClient->listLiveStreams(accessToken, currentLiveStreamIdArray);
	if (liveStreams.empty()) {
		logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", currentLiveStreamId}});
		throw std::runtime_error(
			"YouTubeLiveStreamNotFound(YouTubeStreamSegmenterMainLoop::startContinuousSessionTask)");
	} else if (liveStreams.size() > 1) {
		logger->warn("YouTubeLiveStreamMultipleFound", {{"liveStreamId", currentLiveStreamId}});
	}
	auto currentLiveStream = std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);

	logger->info("YouTubeLiveStreamGottenCurrent", {{"liveStreamId", currentLiveStreamId}});

	// --- Start streaming the initial live broadcast ---
	logger->info("StreamingStarting");

	co_await startStreaming(youTubeApiClient, accessToken, parent, initialLiveBroadcast, currentLiveStream, logger);

	logger->info("StreamingStarted");

	// --- Start completed ---
	logger->info("ContinuousYouTubeSessionStarted");

	co_return {*initialLiveBroadcast, nextLiveBroadcast};
}

Async::Task<void> YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask(
	[[maybe_unused]] Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::YouTubeStore> youtubeStore, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	// on the main thread
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::StopContinuousYouTubeSessionTask");

	logger->info("ContinuousYouTubeSessionStopping");

	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(logger);

	logger->info("OBSStreamingEnsuredStopped");

	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
	// on a worker thread

	// --- YouTube access token ---
	const std::string accessToken = getAccessToken(curl, authStore, logger);

	// --- Complete active broadcasts ---
	logger->info("YouTubeLiveBroadcastCompletingActive");

	const std::array<std::string, 2> liveStreamIds{
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

	// --- Stop completed ---
	logger->info("ContinuousYouTubeSessionStopped");
}

Async::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>>
YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask(
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, YouTubeApi::YouTubeLiveBroadcast incomingLiveBroadcast,
	[[maybe_unused]] QObject *parent, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionSegmenting");

	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};
	// on a worker thread

	const std::string currentLiveStreamId = youtubeStore->getLiveStreamId(currentLiveStreamIndex);
	const std::string incomingLiveStreamId = youtubeStore->getLiveStreamId(1 - currentLiveStreamIndex);
	if (currentLiveStreamId.empty() || incomingLiveStreamId.empty()) {
		logger->error("YouTubeLiveStreamIdNotSet");
		throw std::runtime_error(
			"YouTubeLiveStreamIdNotSet(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	}

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
	const std::string accessToken = getAccessToken(curl, authStore, logger);

	// --- Create the next live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingNext");

	const YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast =
		createLiveBroadcast(youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastNext",
				    "onSetYouTubeThumbnailNext", logger);

	const std::string nextLiveBroadcastId = nextLiveBroadcast.id.value_or("(ID MISSING)");
	const std::string nextLiveBroadcastTitle = (nextLiveBroadcast.snippet && nextLiveBroadcast.snippet->title)
							   ? *nextLiveBroadcast.snippet->title
							   : "(TITLE MISSING)";

	logger->info("YouTubeLiveBroadcastCreatedNext",
		     {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

	// --- Get the switching live stream ---
	logger->info("YouTubeLiveStreamGettingSwitching", {{"liveStreamId", incomingLiveStreamId}});

	const std::array<std::string, 1> incomingLiveStreamIdArray{incomingLiveStreamId};
	const std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
		youTubeApiClient->listLiveStreams(accessToken, incomingLiveStreamIdArray);
	if (liveStreams.empty()) {
		logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", incomingLiveStreamId}});
		throw std::runtime_error(
			"YouTubeLiveStreamNotFound(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	} else if (liveStreams.size() > 1) {
		logger->warn("YouTubeLiveStreamMultipleFound", {{"liveStreamId", incomingLiveStreamId}});
	}
	const auto incomingLiveStream = std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);

	logger->info("YouTubeLiveStreamGottenSwitching", {{"liveStreamId", incomingLiveStream->id}});

	// --- Ensure OBS streaming is stopped ---
	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(logger);

	logger->info("OBSStreamingEnsuredStopped");

	// --- Start streaming the initial live broadcast ---
	logger->info("StreamingStarting");

	auto incomingLiveBroadcastShared = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(incomingLiveBroadcast);
	co_await startStreaming(youTubeApiClient, accessToken, parent, incomingLiveBroadcastShared, incomingLiveStream,
				logger);

	logger->info("StreamingStarted");

	// --- Complete active broadcasts ---
	logger->info("YouTubeLiveBroadcastCompletingActive");

	const std::array<std::string, 2> liveStreamIds{
		currentLiveStreamId,
		incomingLiveStreamId,
	};

	completeActiveLiveBroadcasts(youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Segment completed ---
	logger->info("ContinuousYouTubeSessionSegmented");

	co_return {incomingLiveBroadcast, nextLiveBroadcast};
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
