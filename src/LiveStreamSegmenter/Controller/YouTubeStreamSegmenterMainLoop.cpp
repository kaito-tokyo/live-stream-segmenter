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
#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <stop_token>
#include <string_view>
#include <vector>

#include <QMessageBox>
#include <QObject>

#include <nlohmann/json.hpp>
#include <QCoro/QCoroFuture>
#include <QCoro/QCoroTimer>
#include <QCoro/QCoroThread>
#include <quickjs.h>

#include <obs-frontend-api.h>

#include <KaitoTokyo/Async/Join.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQObject.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQThreadPool.hpp>
#include <KaitoTokyo/AsyncQt/ResumeOnQTimerSingleShot.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>
#include <KaitoTokyo/GoogleAuth/GoogleAuthManager.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>
#include <KaitoTokyo/YouTubeApi/YouTubeTypes.hpp>

#include <EventScriptingContext.hpp>
#include <ScriptingDatabase.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

namespace {

struct EventScriptingContext {
	const std::shared_ptr<JSContext> ctx;
	const std::shared_ptr<Scripting::EventScriptingContext> context;
	const std::shared_ptr<Scripting::ScriptingDatabase> database;

	EventScriptingContext(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
			      std::shared_ptr<const Logger::ILogger> logger, const std::string &scriptContent)
		: ctx(runtime->createContextRaw()),
		  context(std::make_shared<Scripting::EventScriptingContext>(runtime, ctx, logger)),
		  database(std::make_shared<Scripting::ScriptingDatabase>(runtime, ctx, logger, scriptContent, true))
	{
		context->setupContext();
		database->setupContext();
		context->setupLocalStorage();

		context->loadEventHandler(scriptContent.c_str());
	}
};

class TaskBoundLogger : public Logger::ILogger {
public:
	TaskBoundLogger(std::shared_ptr<const Logger::ILogger> baseLogger, std::string_view taskName)
		: baseLogger_(std::move(baseLogger)),
		  taskName_(taskName)
	{
	}

	void log(Logger::LogLevel level, std::string_view name, std::source_location loc,
		 std::span<const Logger::LogField> context) const noexcept override
	{
		std::vector<Logger::LogField> extendedContext;
		extendedContext.reserve(context.size() + 1);
		extendedContext.emplace_back("taskName", taskName_);
		extendedContext.insert(extendedContext.end(), context.begin(), context.end());
		baseLogger_->log(level, name, loc, std::span<const Logger::LogField>(extendedContext));
	}

private:
	std::shared_ptr<const Logger::ILogger> baseLogger_;
	const std::string taskName_;
};

} // anonymous namespace

YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop(
	std::shared_ptr<const Logger::ILogger> logger, std::shared_ptr<Scripting::ScriptingRuntime> runtime,
	std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
	std::shared_ptr<Store::YouTubeStore> youtubeStore, QWidget *parent)
	: QObject(nullptr),
	  logger_(logger ? std::move(logger) : Logger::NullLogger::instance()),
	  runtime_(std::move(runtime)),
	  authStore_(std::move(authStore)),
	  eventHandlerStore_(std::move(eventHandlerStore)),
	  youtubeStore_(std::move(youtubeStore)),
	  parent_(parent),
	  curl_(std::make_shared<CurlHelper::CurlHandle>()),
	  youTubeApiClient_(std::make_shared<YouTubeApi::YouTubeApiClient>(curl_)),
	  tickTimer_(new QTimer(this)),
	  segmentTimer_(new QTimer(this))
{
	assert(logger_);
	if (!runtime_) {
		logger_->error("ScriptingRuntimeIsNullError");
		throw std::invalid_argument(
			"ScriptingRuntimeIsNullError(YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop)");
	}
	if (!authStore_) {
		logger_->error("AuthStoreIsNullError");
		throw std::invalid_argument(
			"AuthStoreIsNullError(YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop)");
	}
	if (!eventHandlerStore_) {
		logger_->error("EventHandlerStoreIsNullError");
		throw std::invalid_argument(
			"EventHandlerStoreIsNullError(YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop)");
	}
	if (!youtubeStore_) {
		logger_->error("YouTubeStoreIsNullError");
		throw std::invalid_argument(
			"YouTubeStoreIsNullError(YouTubeStreamSegmenterMainLoop::YouTubeStreamSegmenterMainLoop)");
	}

	youTubeApiClient_->setLogger(logger_);

	tickTimer_->setTimerType(Qt::VeryCoarseTimer);
	segmentTimer_->setTimerType(Qt::VeryCoarseTimer);

	connect(tickTimer_, &QTimer::timeout, this, [this]() { emit tick(segmentTimer_->remainingTime()); });
	connect(segmentTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession);
}

YouTubeStreamSegmenterMainLoop::~YouTubeStreamSegmenterMainLoop()
{
	stopSource_.request_stop();
	channel_.close();
}

void YouTubeStreamSegmenterMainLoop::startMainLoop()
{
	mainLoopTask_ = mainLoop(stopSource_.get_token(), channel_, curl_, youTubeApiClient_, runtime_, authStore_,
				 eventHandlerStore_, youtubeStore_, logger_, parent_);

	// --- Scripting ---
	std::shared_ptr<JSContext> ctx = runtime_->createContextRaw();
	std::shared_ptr<Scripting::EventScriptingContext> context =
		std::make_shared<Scripting::EventScriptingContext>(runtime_, ctx, logger_);
	Scripting::ScriptingDatabase database(runtime_, ctx, logger_, eventHandlerStore_->getEventHandlerDatabasePath(),
					      true);
	context->setupContext();
	database.setupContext();
	context->setupLocalStorage();

	const std::string scriptContent = eventHandlerStore_->getEventHandlerScript();
	context->loadEventHandler(scriptContent.c_str());

	int segmentIntervalMilliseconds = 60 * 60 * 1000;
	try {
		std::string config = context->executeFunction("onInitYouTubeStreamSegmenter", "{}");
		nlohmann::json jConfig = nlohmann::json::parse(config);
		jConfig.at("segmentIntervalMilliseconds").get_to(segmentIntervalMilliseconds);
	} catch (std::exception &e) {
		logger_->error(
			"YouTubeStreamSegmenterMainLoopScriptError",
			{{"exception", e.what()},
			 {"message",
			  "YouTubeStreamSegmenterMainLoopScriptError: falling back to default interval of 3600000ms"},
			 {"segmentIntervalMilliseconds", "3600000"}});
	}
	tickTimer_->setInterval(1000);
	segmentTimer_->setInterval(segmentIntervalMilliseconds);

	logger_->info("YouTubeStreamSegmenterMainLoopStarted");
}

void YouTubeStreamSegmenterMainLoop::onStartContinuousSession()
{
	tickTimer_->start();
	segmentTimer_->start();
	channel_.send(Message{MessageType::StartContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onStopContinuousSession()
{
	tickTimer_->stop();
	segmentTimer_->stop();
	channel_.send(Message{MessageType::StopContinuousSession});
}

void YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession()
{
	channel_.send(Message{MessageType::SegmentContinuousSession});
}

QCoro::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop(
	Jthread::stop_token stoken, Async::Channel<Message> &channel, std::shared_ptr<CurlHelper::CurlHandle> curl,
	std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
{
	int currentLiveStreamIndex = 0;
	std::array<YouTubeApi::YouTubeLiveBroadcast, 2> liveBroadcasts;
	QThread workerThread;
	workerThread.start();

	while (true) {
		std::optional<Message> message = co_await channel.receive();

		if (!message.has_value()) {
			break;
		}

		try {
			switch (message->type) {
			case MessageType::StartContinuousSession: {
				liveBroadcasts = co_await startContinuousSessionTask(
					parent, &workerThread, stoken, curl, youTubeApiClient, runtime,
					authStore, eventHandlerStore, youtubeStore, currentLiveStreamIndex, logger);
				break;
			}
			case MessageType::StopContinuousSession: {
				co_await stopContinuousSessionTask(parent, &workerThread, stoken, curl,
								   youTubeApiClient, authStore, youtubeStore, logger);
				break;
			}
			case MessageType::SegmentContinuousSession: {
				liveBroadcasts = co_await segmentContinuousSessionTask(
					parent, &workerThread, stoken, curl, youTubeApiClient, runtime, authStore,
					eventHandlerStore, youtubeStore, currentLiveStreamIndex, liveBroadcasts[1],
					logger);
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

// Must be called from a worker thread and returns on a worker thread
std::string getAccessToken(Jthread::stop_token stoken, std::shared_ptr<const Logger::ILogger> logger,
			   std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<Store::AuthStore> authStore)
{
	logger->info("YouTubeAccessTokenGetting");

	std::string accessToken;

	auto clientCredentials = std::make_shared<GoogleAuth::GoogleOAuth2ClientCredentials>(
		authStore->getGoogleOAuth2ClientCredentials());
	GoogleAuth::GoogleAuthManager authManager(logger, curl, clientCredentials);

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

		std::shared_ptr<GoogleAuth::GoogleAuthResponse> freshAuthResponse =
			authManager.fetchFreshAuthResponse(stoken, tokenState.refresh_token);

		tokenState.loadAuthResponse(*freshAuthResponse);

		authStore->setGoogleTokenState(tokenState);

		accessToken = freshAuthResponse->access_token;
		logger->info("YouTubeAccessTokenRefreshed");
	}

	logger->info("YouTubeAccessTokenGotten");

	return accessToken;
}

// Must be called from the main thread and returns on the main thread
QCoro::Task<void> ensureOBSStreamingStopped(Jthread::stop_token stoken, std::shared_ptr<const Logger::ILogger> logger)
{
	if (!obs_frontend_streaming_active()) {
		logger->info("OBSStreamingAlreadyStopped");
		co_return;
	}

	logger->info("OBSStreamingStopping");

	struct StopStreamingAwaiter {
		std::coroutine_handle<> h_;
		Jthread::stop_token stoken_;
		std::optional<Jthread::stop_callback<std::function<void()>>> stopCallback_;
		std::atomic<bool> completed_{false};
		std::atomic<bool> isCancelled_{false};

		static void callback(obs_frontend_event event, void *data)
		{
			if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
				auto *self = static_cast<StopStreamingAwaiter *>(data);
				self->finish(false);
			}
		}

		StopStreamingAwaiter(Jthread::stop_token stoken) : stoken_(stoken) {}

		~StopStreamingAwaiter() { obs_frontend_remove_event_callback(callback, this); }

		void finish(bool cancelled)
		{
			if (!completed_.exchange(true)) {
				isCancelled_ = cancelled;
				obs_frontend_remove_event_callback(callback, this);
				if (h_) {
					h_.resume();
				}
			}
		}

		bool await_ready() const noexcept { return stoken_.stop_requested(); }

		bool await_suspend(std::coroutine_handle<> h) noexcept
		{
			h_ = h;
			obs_frontend_add_event_callback(callback, this);
			obs_frontend_streaming_stop();

			if (!obs_frontend_streaming_active()) {
				obs_frontend_remove_event_callback(callback, this);
				return false;
			}

			stopCallback_.emplace(stoken_, [this]() { finish(true); });

			if (stoken_.stop_requested()) {
				finish(true);
			}

			return true;
		}

		bool await_resume() noexcept
		{
			stopCallback_.reset();
			return !isCancelled_;
		}
	};

	bool success = co_await StopStreamingAwaiter{stoken};

	if (success) {
		logger->info("OBSStreamingStopped");
	} else {
		logger->warn("OBSStreamingStopCancelled");
	}
}

// Must be called from a worker thread and returns on a worker thread
void completeActiveLiveBroadcasts(Jthread::stop_token stoken,
				  std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				  const std::string &accessToken, std::span<const std::string> liveStreamIds,
				  std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCompletingAllActive");

	const std::vector<YouTubeApi::YouTubeLiveBroadcast> activeLiveBroadcasts =
		youTubeApiClient->listLiveBroadcastsByStatus(stoken, accessToken, "active");

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

		youTubeApiClient->transitionLiveBroadcast(stoken, accessToken, liveBroadcastId, "complete");
		logger->info("YouTubeLiveBroadcastCompleted",
			     {{"broadcastId", liveBroadcastId}, {"title", liveBroadcastTitle}});
	}

	logger->info("YouTubeLiveBroadcastCompletedAllActive");
}

// Must be called from a worker thread and returns on a worker thread
YouTubeApi::YouTubeLiveBroadcast
createLiveBroadcast(Jthread::stop_token stoken, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
		    const std::string &accessToken, std::shared_ptr<Scripting::EventScriptingContext> context,
		    const std::string &onCreateLiveBroadcastFunctionName, const std::string &onSetThumbnailFunctionName,
		    std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCreating");

	const std::string result = context->executeFunction(onCreateLiveBroadcastFunctionName.c_str(), R"({})");
	const nlohmann::json j = nlohmann::json::parse(result);
	YouTubeApi::InsertingYouTubeLiveBroadcast insertingLiveBroadcast;
	j.at("YouTubeLiveBroadcast").get_to(insertingLiveBroadcast);

	logger->info("YouTubeLiveBroadcastInserting");

	const YouTubeApi::YouTubeLiveBroadcast liveBroadcast =
		youTubeApiClient->insertLiveBroadcast(stoken, accessToken, insertingLiveBroadcast);
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

			youTubeApiClient->setThumbnail(stoken, accessToken, videoId, thumbnailPath);

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
QCoro::Task<void> startStreaming(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				 const std::string &accessToken, QObject *parent,
				 std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast> nextLiveBroadcast,
				 std::shared_ptr<YouTubeApi::YouTubeLiveStream> nextLiveStream,
				 std::shared_ptr<const Logger::ILogger> logger)
{
	Jthread::stop_token stoken;

	logger->info("StreamingStarting");

	if (!nextLiveBroadcast->id) {
		logger->error("YouTubeLiveBroadcastIdMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIdMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}
	logger->info("YouTubeLiveBroadcastBindingLiveStream",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"streamId", nextLiveStream->id}});

	youTubeApiClient->bindLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, nextLiveStream->id);
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
			youTubeApiClient->listLiveStreams(stoken, accessToken, nextLiveStreamIdArray);

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

	youTubeApiClient->transitionLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, "testing");

	logger->info("YouTubeLiveBroadcastTransitionedToTesting",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});
	co_await AsyncQt::ResumeOnQTimerSingleShot{5000, parent};
	co_await AsyncQt::ResumeOnQThreadPool{QThreadPool::globalInstance()};

	logger->info("YouTubeLiveBroadcastTransitioningToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	youTubeApiClient->transitionLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, "live");

	logger->info("YouTubeLiveBroadcastTransitionedToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});
}

} // anonymous namespace

QCoro::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(
	QObject *parent, QThread *workerThread, Jthread::stop_token stoken,
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	co_await QCoro::moveToThread(parent->thread());
	// on the main thread
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::startContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionStarting");

	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(stoken, logger);

	logger->info("OBSStreamingEnsuredStopped");

	co_await QCoro::moveToThread(workerThread);
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
	const std::string accessToken = getAccessToken(stoken, logger, curl, authStore);

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

	completeActiveLiveBroadcasts(stoken, youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Create an initial live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingInitial");

	auto initialLiveBroadcast = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(
		createLiveBroadcast(stoken, youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastInitial",
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
		createLiveBroadcast(stoken, youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastInitialNext",
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
		youTubeApiClient->listLiveStreams(stoken, accessToken, currentLiveStreamIdArray);
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

QCoro::Task<void> YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask(
	QObject *parent, QThread *workerThread, Jthread::stop_token stoken,
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> baseLogger)
{
	co_await QCoro::moveToThread(parent->thread());
	// on the main thread
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::StopContinuousYouTubeSessionTask");

	logger->info("ContinuousYouTubeSessionStopping");

	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(stoken, logger);

	logger->info("OBSStreamingEnsuredStopped");

	co_await QCoro::moveToThread(workerThread);
	// on a worker thread

	// --- YouTube access token ---
	const std::string accessToken = getAccessToken(stoken, logger, curl, authStore);

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

	completeActiveLiveBroadcasts(stoken, youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Stop completed ---
	logger->info("ContinuousYouTubeSessionStopped");
}

QCoro::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>>
YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask(
	QObject *parent, QThread *workerThread, Jthread::stop_token stoken,
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, YouTubeApi::YouTubeLiveBroadcast incomingLiveBroadcast,
	std::shared_ptr<const Logger::ILogger> baseLogger)
{
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionSegmenting");

	co_await QCoro::moveToThread(workerThread);
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
	const std::string accessToken = getAccessToken(stoken, logger, curl, authStore);

	// --- Create the next live broadcast ---
	logger->info("YouTubeLiveBroadcastCreatingNext");

	const YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast =
		createLiveBroadcast(stoken, youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastNext",
				    "onSetYouTubeThumbnailNext", logger);

	const std::string nextLiveBroadcastId = nextLiveBroadcast.id.value_or("(ID MISSING)");
	const std::string nextLiveBroadcastTitle = (nextLiveBroadcast.snippet && nextLiveBroadcast.snippet->title)
							   ? *nextLiveBroadcast.snippet->title
							   : "(TITLE MISSING)";

	logger->info("YouTubeLiveBroadcastCreatedNext",
		     {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

	// --- Get the incoming live stream ---
	logger->info("YouTubeLiveStreamGettingIncoming", {{"liveStreamId", incomingLiveStreamId}});

	const std::array<std::string, 1> incomingLiveStreamIdArray{incomingLiveStreamId};
	const std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
		youTubeApiClient->listLiveStreams(stoken, accessToken, incomingLiveStreamIdArray);
	if (liveStreams.empty()) {
		logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", incomingLiveStreamId}});
		throw std::runtime_error(
			"YouTubeLiveStreamNotFound(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	} else if (liveStreams.size() > 1) {
		logger->warn("YouTubeLiveStreamMultipleFound", {{"liveStreamId", incomingLiveStreamId}});
	}
	const auto incomingLiveStream = std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);

	logger->info("YouTubeLiveStreamGottenIncoming", {{"liveStreamId", incomingLiveStream->id}});

	// --- Ensure OBS streaming is stopped ---
	logger->info("OBSStreamingEnsuringStopped");

	co_await ensureOBSStreamingStopped(stoken, logger);

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

	completeActiveLiveBroadcasts(stoken, youTubeApiClient, accessToken, liveStreamIds, logger);

	logger->info("YouTubeLiveBroadcastCompletedActive");

	// --- Segment completed ---
	if (!incomingLiveBroadcast.id) {
		logger->error("YouTubeLiveBroadcastIncomingIdMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIncomingIdMissing(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	}
	if (!incomingLiveBroadcast.snippet || !incomingLiveBroadcast.snippet->title) {
		logger->error("YouTubeLiveBroadcastIncomingTitleMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIncomingTitleMissing(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	}
	logger->info("ContinuousYouTubeSessionSegmented",
		     {{"broadcastId", *incomingLiveBroadcast.id}, {"title", *incomingLiveBroadcast.snippet->title}});

	co_return {incomingLiveBroadcast, nextLiveBroadcast};
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
