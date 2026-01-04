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
#include <QtConcurrent> // Required for offloading heavy tasks

#include <quickjs.h>
#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

// QCoro Includes
#include <QCoroFuture>
#include <QCoroSignal>
#include <QCoroTimer>

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

	connect(tickTimer_, &QTimer::timeout, this, [this]() { emit tick(segmentTimer_->remainingTime()); });
	connect(segmentTimer_, &QTimer::timeout, this, &YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession);
}

YouTubeStreamSegmenterMainLoop::~YouTubeStreamSegmenterMainLoop()
{
	// Signal the loop to stop
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		stopRequested_ = true;
	}
	emit messageEnqueued(); // Wake up the loop if it's waiting

	// In QCoro/Qt, we generally rely on the object lifecycle.
	// If we need to wait for the task to finish, we might need a specific mechanism,
	// but usually destructing the QObject cancels pending operations attached to it.
}

void YouTubeStreamSegmenterMainLoop::startMainLoop()
{
	// Start the coroutine. We assign it to a member variable to keep the task alive if needed,
	// though for a "fire and forget" loop on `this`, standard QCoro usage often implies
	// the task runs until completion or cancellation.
	mainLoopTask_ = mainLoop();

	// --- Scripting ---
	// Scripting setup remains on main thread for initialization
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

void YouTubeStreamSegmenterMainLoop::enqueueMessage(MessageType type)
{
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		messageQueue_.push_back(Message{type});
	}
	emit messageEnqueued();
}

void YouTubeStreamSegmenterMainLoop::onStartContinuousSession()
{
	tickTimer_->start();
	segmentTimer_->start();
	enqueueMessage(MessageType::StartContinuousSession);
}

void YouTubeStreamSegmenterMainLoop::onStopContinuousSession()
{
	tickTimer_->stop();
	segmentTimer_->stop();
	enqueueMessage(MessageType::StopContinuousSession);
}

void YouTubeStreamSegmenterMainLoop::onSegmentContinuousSession()
{
	enqueueMessage(MessageType::SegmentContinuousSession);
}

QCoro::Task<void> YouTubeStreamSegmenterMainLoop::mainLoop()
{
	int currentLiveStreamIndex = 0;
	std::array<YouTubeApi::YouTubeLiveBroadcast, 2> liveBroadcasts;

	// Capture 'this' for the coroutine
	auto *self = this;

	while (true) {
		// Wait for message
		while (true) {
			bool hasMessage = false;
			bool stop = false;
			{
				std::lock_guard<std::mutex> lock(self->queueMutex_);
				if (!self->messageQueue_.empty()) {
					hasMessage = true;
				}
				stop = self->stopRequested_;
			}

			if (stop)
				co_return;
			if (hasMessage)
				break;

			// Wait for the signal
			co_await qCoro(self, &YouTubeStreamSegmenterMainLoop::messageEnqueued).waitForNext();
		}

		Message message;
		{
			std::lock_guard<std::mutex> lock(self->queueMutex_);
			if (self->messageQueue_.empty())
				continue; // Spurious wakeup protection
			message = self->messageQueue_.front();
			self->messageQueue_.pop_front();
		}

		try {
			switch (message.type) {
			case MessageType::StartContinuousSession: {
				liveBroadcasts = co_await startContinuousSessionTask(
					curl_, youTubeApiClient_, runtime_, authStore_, eventHandlerStore_,
					youtubeStore_, currentLiveStreamIndex, parent_, logger_);
				break;
			}
			case MessageType::StopContinuousSession: {
				co_await stopContinuousSessionTask(curl_, youTubeApiClient_, authStore_, youtubeStore_,
								   logger_);
				break;
			}
			case MessageType::SegmentContinuousSession: {
				liveBroadcasts = co_await segmentContinuousSessionTask(
					curl_, youTubeApiClient_, runtime_, authStore_, eventHandlerStore_,
					youtubeStore_, currentLiveStreamIndex, liveBroadcasts[1], parent_, logger_);
				currentLiveStreamIndex = (currentLiveStreamIndex + 1) % 2;
				break;
			}
			default:
				logger_->warn("UnknownMessageType");
			}
		} catch (const std::exception &e) {
			logger_->error("MainLoopError", {{"exception", e.what()}});
		} catch (...) {
			logger_->error("MainLoopUnknownError");
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

// Returns on the calling thread (synchronous helper for QtConcurrent)
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

		// Note: Store::AuthStore must be thread-safe as we are calling this from a worker thread
		authStore->setGoogleTokenState(tokenState);

		accessToken = freshAuthResponse.access_token;

		logger->info("YouTubeAccessTokenRefreshed");
	}

	logger->info("YouTubeAccessTokenGotten");

	return accessToken;
}

// Must be called from the main thread
QCoro::Task<void> ensureOBSStreamingStopped(std::shared_ptr<const Logger::ILogger> logger)
{
	if (!obs_frontend_streaming_active()) {
		logger->info("OBSStreamingAlreadyStopped");
		co_return;
	}

	logger->info("OBSStreamingStopping");

	// Standard C++20 Awaiter - Compatible with QCoro
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

// Synchronous helper for QtConcurrent
void completeActiveLiveBroadcasts(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
				  const std::string &accessToken, std::vector<std::string> liveStreamIds,
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

// Synchronous helper for QtConcurrent
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

// Must be called from the main thread because of OBS API usage
QCoro::Task<void> startStreaming(std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
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

	// Note: bindLiveBroadcast is network IO. Ideally this shouldn't block Main Thread,
	// but moving just this call to worker thread while surrounding code needs Main Thread (OBS) is complex.
	// Assuming bindLiveBroadcast is reasonably fast or wrapping it in QtConcurrent if strictly necessary.
	// For this refactor, we keep it consistent with the "Main Thread Logic" block,
	// but heavily network-dependent apps might want to isolate this specific call.
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

	const std::vector<std::string> nextLiveStreamIdArray{nextLiveStream->id};
	for (int maxAttempts = 20; true; --maxAttempts) {
		using namespace std::chrono_literals;
		co_await QCoro::sleepFor(5000ms);

		const std::string maxAttemptsStr = std::to_string(maxAttempts);
		logger->info("YouTubeLiveStreamCheckingIfActive",
			     {{"liveStreamId", nextLiveStream->id}, {"attemptsLeft", maxAttemptsStr}});

		// Network call on main thread (see previous note)
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

	using namespace std::chrono_literals;
	co_await QCoro::sleepFor(5000ms);

	logger->info("YouTubeLiveBroadcastTransitioningToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	youTubeApiClient->transitionLiveBroadcast(accessToken, *nextLiveBroadcast->id, "live");

	logger->info("YouTubeLiveBroadcastTransitionedToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});
}

} // anonymous namespace

QCoro::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>> YouTubeStreamSegmenterMainLoop::startContinuousSessionTask(
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, QObject *parent, std::shared_ptr<const Logger::ILogger> baseLogger)
{
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::startContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionStarting");
	logger->info("OBSStreamingEnsuringStopped");

	// 1. Stop OBS (Must be on Main Thread)
	co_await ensureOBSStreamingStopped(logger);

	logger->info("OBSStreamingEnsuredStopped");

	// 2. Heavy Lifting (Network/Scripting) -> Offload to Worker Thread via QtConcurrent
	// We gather all the data needed for the next step here
	auto workerResult = co_await QtConcurrent::run([=]() -> std::pair<YouTubeApi::YouTubeLiveBroadcast,
									  YouTubeApi::YouTubeLiveBroadcast> {
		// --- Scripting Setup ---
		std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
		std::shared_ptr<Scripting::EventScriptingContext> context =
			std::make_shared<Scripting::EventScriptingContext>(runtime, ctx, logger);
		Scripting::ScriptingDatabase database(runtime, ctx, logger,
						      eventHandlerStore->getEventHandlerDatabasePath(), true);
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

		const std::vector<std::string> liveStreamIds{
			currentLiveStreamId,
			nextLiveStreamId,
		};

		completeActiveLiveBroadcasts(youTubeApiClient, accessToken, liveStreamIds, logger);

		logger->info("YouTubeLiveBroadcastCompletedActive");

		// --- Create an initial live broadcast ---
		logger->info("YouTubeLiveBroadcastCreatingInitial");

		auto initialLiveBroadcast = createLiveBroadcast(youTubeApiClient, accessToken, context,
								"onCreateYouTubeLiveBroadcastInitial",
								"onSetYouTubeThumbnailInitial", logger);

		const std::string initialLiveBroadcastId = initialLiveBroadcast.id.value_or("(ID MISSING)");
		const std::string initialLiveBroadcastTitle =
			(initialLiveBroadcast.snippet && initialLiveBroadcast.snippet->title)
				? *initialLiveBroadcast.snippet->title
				: "(TITLE MISSING)";
		logger->info("YouTubeLiveBroadcastCreatedInitial",
			     {{"broadcastId", initialLiveBroadcastId}, {"title", initialLiveBroadcastTitle}});

		// --- Create the next live broadcast ---
		logger->info("YouTubeLiveBroadcastCreatingNext");

		const YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast = createLiveBroadcast(
			youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastInitialNext",
			"onSetYouTubeThumbnailInitialNext", logger);

		const std::string nextLiveBroadcastId = nextLiveBroadcast.id.value_or("(ID MISSING)");
		const std::string nextLiveBroadcastTitle =
			(nextLiveBroadcast.snippet && nextLiveBroadcast.snippet->title)
				? *nextLiveBroadcast.snippet->title
				: "(TITLE MISSING)";

		logger->info("YouTubeLiveBroadcastCreatedNext",
			     {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

		return {initialLiveBroadcast, nextLiveBroadcast};
	});

	auto initialLiveBroadcast = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(workerResult.first);
	auto nextLiveBroadcast = workerResult.second;

	// 3. Prepare for Streaming (Requires Network + Main Thread for OBS later)
	// We need the Access Token again or pass it out from the worker.
	// For simplicity, we re-fetch (it's cached/fresh now) or we should have returned it.
	// Let's re-fetch quickly as it handles refresh logic safely.
	// However, `getAccessToken` is sync and might block main thread.
	// Better approach: Return accessToken from the worker lambda above.
	// *Correction*: To avoid large refactors of signatures, we assume token is fresh enough or acceptable to call sync
	// because `QtConcurrent` just refreshed it.
	// But `getAccessToken` performs HTTP. Let's do a quick re-fetch in a mini worker task if needed,
	// or rely on the fact that AuthStore now has the token.

	// Retrieving ID for the Stream (lightweight)
	const std::string currentLiveStreamId = youtubeStore->getLiveStreamId(currentLiveStreamIndex);

	// We need the stream details. This is network IO. Let's offload this small part too.
	auto currentLiveStream = co_await QtConcurrent::run([=]() -> std::shared_ptr<YouTubeApi::YouTubeLiveStream> {
		const std::string token =
			authStore->getGoogleTokenState().access_token; // Assume valid from previous step
		const std::vector<std::string> currentLiveStreamIdArray{currentLiveStreamId};

		logger->info("YouTubeLiveStreamGettingCurrent", {{"liveStreamId", currentLiveStreamId}});
		auto liveStreams = youTubeApiClient->listLiveStreams(token, currentLiveStreamIdArray);

		if (liveStreams.empty()) {
			logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", currentLiveStreamId}});
			throw std::runtime_error("YouTubeLiveStreamNotFound");
		}

		logger->info("YouTubeLiveStreamGottenCurrent", {{"liveStreamId", currentLiveStreamId}});
		return std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);
	});

	// 4. Start Streaming (Main Thread - touches OBS)
	const std::string accessToken = authStore->getGoogleTokenState().access_token;

	logger->info("StreamingStarting");
	co_await startStreaming(youTubeApiClient, accessToken, parent, initialLiveBroadcast, currentLiveStream, logger);
	logger->info("StreamingStarted");

	logger->info("ContinuousYouTubeSessionStarted");

	co_return {*initialLiveBroadcast, nextLiveBroadcast};
}

QCoro::Task<void> YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask(
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::shared_ptr<const Logger::ILogger> baseLogger)
{
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::StopContinuousYouTubeSessionTask");

	logger->info("ContinuousYouTubeSessionStopping");
	logger->info("OBSStreamingEnsuringStopped");

	// 1. Stop OBS (Main Thread)
	co_await ensureOBSStreamingStopped(logger);
	logger->info("OBSStreamingEnsuredStopped");

	// 2. Network cleanup (Worker Thread)
	co_await QtConcurrent::run([=]() {
		const std::string accessToken = getAccessToken(curl, authStore, logger);

		logger->info("YouTubeLiveBroadcastCompletingActive");

		const std::vector<std::string> liveStreamIds{
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
	});

	logger->info("ContinuousYouTubeSessionStopped");
}

QCoro::Task<std::array<YouTubeApi::YouTubeLiveBroadcast, 2>>
YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask(
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore,
	std::size_t currentLiveStreamIndex, YouTubeApi::YouTubeLiveBroadcast incomingLiveBroadcast, QObject *parent,
	std::shared_ptr<const Logger::ILogger> baseLogger)
{
	const std::shared_ptr<const Logger::ILogger> logger = std::make_shared<TaskBoundLogger>(
		baseLogger, "YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask");

	logger->info("ContinuousYouTubeSessionSegmenting");

	const std::string currentLiveStreamId = youtubeStore->getLiveStreamId(currentLiveStreamIndex);
	const std::string incomingLiveStreamId = youtubeStore->getLiveStreamId(1 - currentLiveStreamIndex);

	if (currentLiveStreamId.empty() || incomingLiveStreamId.empty()) {
		logger->error("YouTubeLiveStreamIdNotSet");
		throw std::runtime_error(
			"YouTubeLiveStreamIdNotSet(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
	}

	// 1. Heavy Work (Scripting, API creation) -> Worker Thread
	auto workerResult = co_await QtConcurrent::run([=]() -> std::pair<
								     YouTubeApi::YouTubeLiveBroadcast,
								     std::shared_ptr<YouTubeApi::YouTubeLiveStream>> {
		// Scripting Init
		std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
		std::shared_ptr<Scripting::EventScriptingContext> context =
			std::make_shared<Scripting::EventScriptingContext>(runtime, ctx, logger);
		Scripting::ScriptingDatabase database(runtime, ctx, logger,
						      eventHandlerStore->getEventHandlerDatabasePath(), true);
		context->setupContext();
		database.setupContext();
		context->setupLocalStorage();

		const std::string scriptContent = eventHandlerStore->getEventHandlerScript();
		context->loadEventHandler(scriptContent.c_str());

		// Token
		const std::string accessToken = getAccessToken(curl, authStore, logger);

		// Create Next Broadcast
		logger->info("YouTubeLiveBroadcastCreatingNext");
		const YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast =
			createLiveBroadcast(youTubeApiClient, accessToken, context, "onCreateYouTubeLiveBroadcastNext",
					    "onSetYouTubeThumbnailNext", logger);

		const std::string nextLiveBroadcastId = nextLiveBroadcast.id.value_or("(ID MISSING)");
		const std::string nextLiveBroadcastTitle =
			(nextLiveBroadcast.snippet && nextLiveBroadcast.snippet->title)
				? *nextLiveBroadcast.snippet->title
				: "(TITLE MISSING)";

		logger->info("YouTubeLiveBroadcastCreatedNext",
			     {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

		// Get Incoming Stream Info
		logger->info("YouTubeLiveStreamGettingIncoming", {{"liveStreamId", incomingLiveStreamId}});

		const std::vector<std::string> incomingLiveStreamIdArray{incomingLiveStreamId};
		const std::vector<YouTubeApi::YouTubeLiveStream> liveStreams =
			youTubeApiClient->listLiveStreams(accessToken, incomingLiveStreamIdArray);

		if (liveStreams.empty()) {
			logger->error("YouTubeLiveStreamNotFound", {{"liveStreamId", incomingLiveStreamId}});
			throw std::runtime_error(
				"YouTubeLiveStreamNotFound(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
		}

		auto incomingLiveStream = std::make_shared<YouTubeApi::YouTubeLiveStream>(liveStreams[0]);
		logger->info("YouTubeLiveStreamGottenIncoming", {{"liveStreamId", incomingLiveStream->id}});

		return {nextLiveBroadcast, incomingLiveStream};
	});

	YouTubeApi::YouTubeLiveBroadcast nextLiveBroadcast = workerResult.first;
	auto incomingLiveStream = workerResult.second;

	// 2. Stop OBS (Main Thread)
	logger->info("OBSStreamingEnsuringStopped");
	co_await ensureOBSStreamingStopped(logger);
	logger->info("OBSStreamingEnsuredStopped");

	// 3. Start Streaming (Main Thread)
	logger->info("StreamingStarting");
	auto incomingLiveBroadcastShared = std::make_shared<YouTubeApi::YouTubeLiveBroadcast>(incomingLiveBroadcast);

	const std::string accessToken = authStore->getGoogleTokenState().access_token;

	co_await startStreaming(youTubeApiClient, accessToken, parent, incomingLiveBroadcastShared, incomingLiveStream,
				logger);
	logger->info("StreamingStarted");

	// 4. Complete Old Broadcasts (Worker Thread)
	// We run this detached or awaited? Logic suggests awaited to ensure clean state before returning
	co_await QtConcurrent::run([=]() {
		const std::string token = authStore->getGoogleTokenState().access_token;
		logger->info("YouTubeLiveBroadcastCompletingActive");
		const std::vector<std::string> liveStreamIds{
			currentLiveStreamId,
			incomingLiveStreamId,
		};
		completeActiveLiveBroadcasts(youTubeApiClient, token, liveStreamIds, logger);
		logger->info("YouTubeLiveBroadcastCompletedActive");
	});

	if (!incomingLiveBroadcast.id) {
		logger->error("YouTubeLiveBroadcastIncomingIdMissing");
		throw std::runtime_error("YouTubeLiveBroadcastIncomingIdMissing");
	}
	if (!incomingLiveBroadcast.snippet || !incomingLiveBroadcast.snippet->title) {
		logger->error("YouTubeLiveBroadcastIncomingTitleMissing");
		throw std::runtime_error("YouTubeLiveBroadcastIncomingTitleMissing");
	}
	logger->info("ContinuousYouTubeSessionSegmented",
		     {{"broadcastId", *incomingLiveBroadcast.id}, {"title", *incomingLiveBroadcast.snippet->title}});

	co_return {incomingLiveBroadcast, nextLiveBroadcast};
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
