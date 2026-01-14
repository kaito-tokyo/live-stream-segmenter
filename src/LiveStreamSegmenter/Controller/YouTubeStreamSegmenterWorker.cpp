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

#include "YouTubeStreamSegmenterWorker.hpp"

#include <chrono>

#include <QCoro/QCoroThread>
#include <QCoro/QCoroTimer>

#include <obs-frontend-api.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <KaitoTokyo/GoogleAuth/GoogleAuthManager.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>
#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

#include <EventHandlerStore.hpp>
#include <EventScriptingContext.hpp>
#include <ScriptingDatabase.hpp>
#include <ScriptingRuntime.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

namespace {

struct EventScriptingContext {
	const std::shared_ptr<JSContext> ctx;
	const std::shared_ptr<Scripting::EventScriptingContext> context;
	const std::shared_ptr<Scripting::ScriptingDatabase> database;

	EventScriptingContext(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
			      std::shared_ptr<const Logger::ILogger> logger,
			      std::shared_ptr<Store::EventHandlerStore> eventHandlerStore)
		: ctx(runtime->createContextRaw()),
		  context(std::make_shared<Scripting::EventScriptingContext>(runtime, ctx, logger)),
		  database(std::make_shared<Scripting::ScriptingDatabase>(
			  runtime, ctx, logger, eventHandlerStore->getEventHandlerDatabasePath(), true))
	{
		context->setupContext();
		database->setupContext();
		context->setupLocalStorage();

		std::string scriptContent = eventHandlerStore->getEventHandlerScript();
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

	const std::variant<std::vector<std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>>,
			   std::shared_ptr<YouTubeApi::YouTubeApiError>>
		apiResult = youTubeApiClient->listLiveBroadcastsByStatus(stoken, accessToken, "active");

	if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
		const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
		logger->error("YouTubeApiError", {{"code", error->code}, {"message", error->message}});
		throw std::runtime_error(
			"YouTubeApiError(YouTubeStreamSegmenterMainLoop::completeActiveLiveBroadcasts)");
	}

	const auto &activeLiveBroadcasts =
		std::get<std::vector<std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>>>(apiResult);

	for (const auto &liveBroadcast : activeLiveBroadcasts) {
		if (!liveBroadcast->contentDetails || !liveBroadcast->contentDetails->boundStreamId) {
			logger->warn("YouTubeLiveBroadcastBoundStreamIdMissing");
			continue;
		}

		const std::string &boundStreamId = *liveBroadcast->contentDetails->boundStreamId;

		const auto it = std::ranges::find(liveStreamIds, boundStreamId);
		if (it == liveStreamIds.end())
			continue;

		if (!liveBroadcast->id) {
			logger->warn("YouTubeLiveBroadcastIdMissing");
			continue;
		}

		const std::string &liveBroadcastId = *liveBroadcast->id;
		const std::string liveBroadcastTitle = liveBroadcast->snippet && liveBroadcast->snippet->title
							       ? *liveBroadcast->snippet->title
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
std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>
createLiveBroadcast(Jthread::stop_token stoken, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
		    const std::string &accessToken, std::shared_ptr<Scripting::EventScriptingContext> context,
		    const std::string &onCreateLiveBroadcastFunctionName, const std::string &onSetThumbnailFunctionName,
		    std::shared_ptr<const Logger::ILogger> logger)
{
	logger->info("YouTubeLiveBroadcastCreating");

	const std::string resultStr = context->executeFunction(onCreateLiveBroadcastFunctionName.c_str(), R"({})");
	const nlohmann::json j = nlohmann::json::parse(resultStr);
	YouTubeApi::InsertingYouTubeLiveBroadcast insertingLiveBroadcast;
	j.at("YouTubeLiveBroadcast").get_to(insertingLiveBroadcast);

	logger->info("YouTubeLiveBroadcastInserting");

	const std::variant<std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>,
			   std::shared_ptr<YouTubeApi::YouTubeApiError>>
		apiResult = youTubeApiClient->insertLiveBroadcast(stoken, accessToken, insertingLiveBroadcast);

	if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
		const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
		logger->error("YouTubeApiError", {{"code", error->code}, {"message", error->message}});
		throw std::runtime_error("YouTubeApiError(YouTubeStreamSegmenterMainLoop::createLiveBroadcast)");
	}

	const auto &liveBroadcast = std::get<std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast>>(apiResult);

	const std::string liveBroadcastId = liveBroadcast->id.value_or("(ID MISSING)");
	const std::string liveBroadcastTitle = (liveBroadcast->snippet && liveBroadcast->snippet->title)
						       ? *liveBroadcast->snippet->title
						       : "(TITLE MISSING)";
	logger->info("YouTubeLiveBroadcastInserted", {{"broadcastId", liveBroadcastId}, {"title", liveBroadcastTitle}});

	const nlohmann::json setThumbnailEventObj{
		{"LiveBroadcast", *liveBroadcast},
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

// Must be called from the main thread and returns on a worker thread
QCoro::Task<YouTubeStreamSegmenterWorker::CurrentLiveBroadcast>
startStreaming(QThread *workerThread, Jthread::stop_token stoken,
	       std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient, const std::string &accessToken,
	       [[maybe_unused]] QObject *parent, std::shared_ptr<YouTubeApi::YouTubeLiveBroadcast> nextLiveBroadcast,
	       std::shared_ptr<const Logger::ILogger> logger)
{
	using namespace std::chrono_literals;

	logger->info("StreamingStarting");

	// --- Create a new live stream ---
	logger->info("YouTubeLiveStreamInserting");
	const auto insertingLiveStream = std::make_shared<YouTubeApi::InsertingYouTubeLiveStream>();
	insertingLiveStream->snippet.title = fmt::format("Live Stream Segmenter {}", std::chrono::system_clock::now());
	insertingLiveStream->cdn.ingestionType = "hls";
	insertingLiveStream->cdn.frameRate = "variable";
	insertingLiveStream->cdn.resolution = "variable";
	YouTubeApi::InsertingYouTubeLiveStream::ContentDetails contentDetails;
	contentDetails.isReusable = false;
	insertingLiveStream->contentDetails = std::move(contentDetails);

	const std::variant<std::shared_ptr<YouTubeApi::YouTubeLiveStream>, std::shared_ptr<YouTubeApi::YouTubeApiError>>
		apiResult = youTubeApiClient->insertLiveStream(stoken, accessToken, insertingLiveStream);

	if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
		const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
		logger->error("YouTubeApiError", {{"code", error->code}, {"message", error->message}});
		throw std::runtime_error("YouTubeApiError(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}

	const auto &nextLiveStream = std::get<std::shared_ptr<YouTubeApi::YouTubeLiveStream>>(apiResult);

	logger->info("YouTubeLiveStreamInserted");

	if (!nextLiveBroadcast->id) {
		logger->error("YouTubeLiveBroadcastIdMissing");
		throw std::runtime_error(
			"YouTubeLiveBroadcastIdMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}

	std::string nextLiveStreamId = nextLiveStream->id.value_or("(ID MISSING)");

	logger->info("YouTubeLiveBroadcastBindingLiveStream",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"streamId", nextLiveStreamId}});

	youTubeApiClient->bindLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, nextLiveStreamId);
	logger->info("YouTubeLiveBroadcastBoundToLiveStream",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"streamId", nextLiveStreamId}});

	if (!nextLiveStream->cdn) {
		logger->error("YouTubeLiveStreamCdnMissing");
		throw std::runtime_error("YouTubeLiveStreamCdnMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}

	if (!nextLiveStream->cdn->ingestionType) {
		logger->error("YouTubeLiveStreamIngestionTypeMissing");
		throw std::runtime_error(
			"YouTubeLiveStreamIngestionTypeMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}

	const std::string &nextLiveStreamIngestionType = *nextLiveStream->cdn->ingestionType;

	if (!nextLiveStream->cdn->ingestionInfo || !nextLiveStream->cdn->ingestionInfo->streamName) {
		logger->error("YouTubeLiveStreamIngestionInfoStreamNameMissing");
		throw std::runtime_error(
			"YouTubeLiveStreamIngestionInfoStreamNameMissing(YouTubeStreamSegmenterMainLoop::startStreaming)");
	}

	const std::string nextLiveStreamStreamName = *nextLiveStream->cdn->ingestionInfo->streamName;

	if (nextLiveStreamIngestionType == "rtmp") {
		logger->info("OBSStreamingYouTubeRTMPServiceCreating");

		auto settings = ObsBridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "Custom");
		obs_data_set_string(settings.get(), "protocol", "RTMPS");
		obs_data_set_string(settings.get(), "server", "rtmps://a.rtmps.youtube.com:443/live2");
		obs_data_set_string(settings.get(), "key", nextLiveStreamStreamName.c_str());

		obs_service_t *service = obs_service_create("rtmp_common", "Live Stream Segmenter YouTube RTMP Service",
							    settings.get(), nullptr);

		obs_frontend_set_streaming_service(service);
		obs_service_release(service);

		logger->info("OBSStreamingYouTubeRTMPServiceCreated");
	} else if (nextLiveStreamIngestionType == "hls") {
		logger->info("OBSStreamingYouTubeHLSServiceCreating");

		auto settings = ObsBridgeUtils::unique_obs_data_t(obs_data_create());
		obs_data_set_string(settings.get(), "service", "Custom");
		obs_data_set_string(settings.get(), "protocol", "HLS");
		obs_data_set_string(
			settings.get(), "server",
			"https://a.upload.youtube.com/http_upload_hls?cid={stream_key}&copy=0&file=out.m3u8");
		obs_data_set_string(settings.get(), "key", nextLiveStreamStreamName.c_str());

		obs_service_t *service = obs_service_create("rtmp_common", "Live Stream Segmenter YouTube HLS Service",
							    settings.get(), nullptr);

		obs_frontend_set_streaming_service(service);
		obs_service_release(service);

		logger->info("OBSStreamingYouTubeHLSServiceCreated");
	} else {
		logger->error("OBSStreamingUnsupportedYouTubeIngestionTypeError",
			      {{"ingestionType", nextLiveStreamIngestionType}});
		throw std::runtime_error(
			"OBSStreamingUnsupportedYouTubeIngestionTypeError(YouTubeStreamSegmenterMainLoop::startOBSStreaming)");
	}

	obs_frontend_streaming_start();

	logger->info("OBSStreamingStarted");

	logger->info("YouTubeLiveStreamWaitingForActive", {{"liveStreamId", nextLiveStreamId}});
	co_await QCoro::moveToThread(workerThread);
	// on a worker thread

	const std::array<std::string, 1> nextLiveStreamIdArray{nextLiveStreamId};
	for (int maxAttempts = 20; true; --maxAttempts) {
		co_await QCoro::sleepFor(5s);

		const std::string maxAttemptsStr = std::to_string(maxAttempts);
		logger->info("YouTubeLiveStreamCheckingIfActive",
			     {{"liveStreamId", nextLiveStreamId}, {"attemptsLeft", maxAttemptsStr}});

		const std::variant<std::vector<std::shared_ptr<YouTubeApi::YouTubeLiveStream>>,
				   std::shared_ptr<YouTubeApi::YouTubeApiError>>
			apiResult = youTubeApiClient->listLiveStreams(stoken, accessToken, nextLiveStreamIdArray);

		if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
			const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
			logger->error("YouTubeApiError", {{"code", error->code}, {"message", error->message}});
			throw std::runtime_error("YouTubeApiError(YouTubeStreamSegmenterMainLoop::startStreaming)");
		}

		const auto &liveStreams =
			std::get<std::vector<std::shared_ptr<YouTubeApi::YouTubeLiveStream>>>(apiResult);

		if (liveStreams.size() == 1 && liveStreams[0]->status.has_value() &&
		    liveStreams[0]->status->streamStatus == "active") {
			logger->info("YouTubeLiveStreamActive", {{"liveStreamId", nextLiveStreamId}});
			break;
		}

		if (maxAttempts <= 0) {
			logger->error("YouTubeLiveStreamTimeout", {{"liveStreamId", nextLiveStreamId}});
			throw std::runtime_error(
				"YouTubeLiveStreamTimeout(YouTubeStreamSegmenterMainLoop::startStreaming)");
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

	// Wait a bit before transitioning to "testing"
	co_await QCoro::sleepFor(5s);

	youTubeApiClient->transitionLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, "testing");

	logger->info("YouTubeLiveBroadcastTransitionedToTesting",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	logger->info("YouTubeLiveBroadcastTransitioningToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	// Wait a bit before transitioning to "live"
	co_await QCoro::sleepFor(5s);

	youTubeApiClient->transitionLiveBroadcast(stoken, accessToken, *nextLiveBroadcast->id, "live");

	logger->info("YouTubeLiveBroadcastTransitionedToLive",
		     {{"broadcastId", *nextLiveBroadcast->id}, {"title", nextLiveBroadcastTitle}});

	YouTubeStreamSegmenterWorker::CurrentLiveBroadcast currentLiveBroadcast;
	currentLiveBroadcast.liveBroadcast = nextLiveBroadcast;
	currentLiveBroadcast.liveStream = nextLiveStream;

	co_return currentLiveBroadcast;
}

} // anonymous namespace

YouTubeStreamSegmenterWorker::YouTubeStreamSegmenterWorker(
	QObject *mainContext, QThread *workerThread, std::shared_ptr<const Logger::ILogger> logger,
	std::shared_ptr<CurlHelper::CurlHandle> curl, std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient,
	std::shared_ptr<Scripting::ScriptingRuntime> runtime, std::shared_ptr<Store::AuthStore> authStore,
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore, std::shared_ptr<Store::YouTubeStore> youtubeStore)
	: QObject(nullptr),
	  mainContext_(mainContext),
	  workerThread_(workerThread),
	  logger_(logger ? std::move(logger) : Logger::NullLogger::instance()),
	  curl_(std::move(curl)),
	  youTubeApiClient_(std::move(youTubeApiClient)),
	  runtime_(std::move(runtime)),
	  authStore_(std::move(authStore)),
	  eventHandlerStore_(std::move(eventHandlerStore)),
	  youtubeStore_(std::move(youtubeStore))
{
	if (!mainContext_) {
		throw std::invalid_argument("MainContextIsNullError(YouTubeStreamSegmenterWorker)");
	}
	assert(logger_);
	if (!curl_) {
		throw std::invalid_argument("CurlIsNullError(YouTubeStreamSegmenterWorker)");
	}
	if (!youTubeApiClient_) {
		throw std::invalid_argument("YouTubeApiClientIsNullError(YouTubeStreamSegmenterWorker)");
	}
	if (!runtime_) {
		throw std::invalid_argument("ScriptingRuntimeIsNullError(YouTubeStreamSegmenterWorker)");
	}
	if (!authStore_) {
		throw std::invalid_argument("AuthStoreIsNullError(YouTubeStreamSegmenterWorker)");
	}
	if (!eventHandlerStore_) {
		throw std::invalid_argument("EventHandlerStoreIsNullError(YouTubeStreamSegmenterWorker)");
	}
	if (!youtubeStore_) {
		throw std::invalid_argument("YouTubeStoreIsNullError(YouTubeStreamSegmenterWorker)");
	}
}

YouTubeStreamSegmenterWorker::~YouTubeStreamSegmenterWorker() noexcept {}

QCoro::Task<> YouTubeStreamSegmenterWorker::onStartSession()
{
	std::shared_ptr<const Logger::ILogger> taskLogger = Logger::NullLogger::instance();

	try {
		taskLogger = std::make_shared<TaskBoundLogger>(
			logger_, "YouTubeStreamSegmenterMainLoop::startContinuousSessionTask");

		if (stopSource_.stop_requested()) {
			stopSource_ = Jthread::stop_source();
		}
		Jthread::stop_token stoken = stopSource_.get_token();

		taskLogger->info("ContinuousYouTubeSessionStarting");

		// --- Stopping OBS streaming ---
		taskLogger->info("OBSStreamingEnsuringStopped");

		if (QThread *mainThread = mainContext_->thread()) {
			co_await QCoro::moveToThread(mainThread);
		} else {
			throw std::runtime_error(
				"MainContextHasNoThreadError(YouTubeStreamSegmenterWorker::onStartSession)");
		}
		// on the main thread

		co_await ensureOBSStreamingStopped(stoken, taskLogger);

		taskLogger->info("OBSStreamingEnsuredStopped");

		// --- Initializing scripting context ---
		co_await QCoro::moveToThread(workerThread_);
		// on a worker thread

		EventScriptingContext eventScriptingContext(runtime_, taskLogger, eventHandlerStore_);

		// --- YouTube access token ---
		const std::string accessToken = getAccessToken(stoken, logger_, curl_, authStore_);

		// --- Create an initial live broadcast ---
		taskLogger->info("YouTubeLiveBroadcastCreatingInitial");

		auto initialLiveBroadcast = createLiveBroadcast(stoken, youTubeApiClient_, accessToken,
								eventScriptingContext.context,
								"onCreateYouTubeLiveBroadcastInitial",
								"onSetYouTubeThumbnailInitial", taskLogger);

		const std::string initialLiveBroadcastId = initialLiveBroadcast->id.value_or("(ID MISSING)");
		const std::string initialLiveBroadcastTitle =
			(initialLiveBroadcast->snippet && initialLiveBroadcast->snippet->title)
				? *initialLiveBroadcast->snippet->title
				: "(TITLE MISSING)";
		taskLogger->info("YouTubeLiveBroadcastCreatedInitial",
				 {{"broadcastId", initialLiveBroadcastId}, {"title", initialLiveBroadcastTitle}});

		// --- Create the next live broadcast ---
		taskLogger->info("YouTubeLiveBroadcastCreatingNext");

		const auto nextLiveBroadcast = createLiveBroadcast(stoken, youTubeApiClient_, accessToken,
								   eventScriptingContext.context,
								   "onCreateYouTubeLiveBroadcastInitialNext",
								   "onSetYouTubeThumbnailInitialNext", taskLogger);

		const std::string nextLiveBroadcastId = nextLiveBroadcast->id.value_or("(ID MISSING)");
		const std::string nextLiveBroadcastTitle =
			(nextLiveBroadcast->snippet && nextLiveBroadcast->snippet->title)
				? *nextLiveBroadcast->snippet->title
				: "(TITLE MISSING)";

		taskLogger->info("YouTubeLiveBroadcastCreatedNext",
				 {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

		// --- Start streaming the initial live broadcast ---
		taskLogger->info("StreamingStarting");

		if (QThread *mainThread = mainContext_->thread()) {
			co_await QCoro::moveToThread(mainThread);
		} else {
			throw std::runtime_error(
				"MainContextHasNoThreadError(YouTubeStreamSegmenterWorker::onStartSession)");
		}
		// on the main thread

		currentLiveBroadcast_ = co_await startStreaming(workerThread_, stoken, youTubeApiClient_, accessToken,
								mainContext_, initialLiveBroadcast, taskLogger);
		// on a worker thread

		taskLogger->info("StreamingStarted");

		// --- Start completed ---
		taskLogger->info("ContinuousYouTubeSessionStarted");

		liveBroadcasts_[0] = initialLiveBroadcast;
		liveBroadcasts_[1] = nextLiveBroadcast;

		emit sessionStarted();
	} catch (const std::exception &e) {
		taskLogger->error("ContinuousYouTubeSessionStartFailed", {{"error", e.what()}});
		QString errStr = e.what();
		emit errorOccurred(errStr);
	} catch (...) {
		taskLogger->error("ContinuousYouTubeSessionStartFailedUnknownError");
		emit errorOccurred("Unknown error");
	}
}

QCoro::Task<> YouTubeStreamSegmenterWorker::onStopSession()
{
	std::shared_ptr<const Logger::ILogger> taskLogger = Logger::NullLogger::instance();

	try {
		taskLogger = std::make_shared<TaskBoundLogger>(
			logger_, "YouTubeStreamSegmenterMainLoop::StopContinuousYouTubeSessionTask");

		if (stopSource_.stop_requested()) {
			stopSource_ = Jthread::stop_source();
		}
		Jthread::stop_token stoken = stopSource_.get_token();

		taskLogger->info("ContinuousYouTubeSessionStopping");

		// --- Stopping OBS streaming ---

		taskLogger->info("OBSStreamingEnsuringStopped");

		if (QThread *mainThread = mainContext_->thread()) {
			co_await QCoro::moveToThread(mainThread);
		} else {
			throw std::runtime_error(
				"MainContextHasNoThreadError(YouTubeStreamSegmenterWorker::onStartSession)");
		}
		// on the main thread

		co_await ensureOBSStreamingStopped(stoken, taskLogger);

		taskLogger->info("OBSStreamingEnsuredStopped");

		// --- YouTube access token ---
		co_await QCoro::moveToThread(workerThread_);
		// on a worker thread

		const std::string accessToken = getAccessToken(stoken, taskLogger, curl_, authStore_);

		// --- Complete active broadcasts ---
		if (currentLiveBroadcast_ && currentLiveBroadcast_->liveStream &&
		    currentLiveBroadcast_->liveStream->id) {
			std::string currentLiveStreamId = *currentLiveBroadcast_->liveStream->id;

			taskLogger->info("YouTubeLiveBroadcastCompletingActive");

			const std::array<std::string, 1> liveStreamIds{currentLiveStreamId};

			completeActiveLiveBroadcasts(stoken, youTubeApiClient_, accessToken, liveStreamIds, taskLogger);

			taskLogger->info("YouTubeLiveBroadcastCompletedActive");

			taskLogger->info("YouTubeLiveStreamDeleting", {{"liveStreamId", currentLiveStreamId}});

			const std::variant<std::shared_ptr<YouTubeApi::YouTubeLiveStream>,
					   std::shared_ptr<YouTubeApi::YouTubeApiError>>
				apiResult =
					youTubeApiClient_->deleteLiveStream(stoken, accessToken, currentLiveStreamId);

			if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
				const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
				taskLogger->error("YouTubeApiError",
						  {{"code", error->code}, {"message", error->message}});
				throw std::runtime_error(
					"YouTubeApiError(YouTubeStreamSegmenterMainLoop::stopContinuousSessionTask)");
			}

			taskLogger->info("YouTubeLiveStreamDeleted", {{"liveStreamId", currentLiveStreamId}});
		} else {
			taskLogger->info("YouTubeLiveBroadcastSkippingCompleteActiveNoCurrentBroadcast");
		}

		// --- Stop completed ---
		taskLogger->info("ContinuousYouTubeSessionStopped");

		emit sessionStopped();
	} catch (const std::exception &e) {
		taskLogger->error("ContinuousYouTubeSessionStopFailed", {{"error", e.what()}});
		QString errStr = e.what();
		emit errorOccurred(errStr);
	} catch (...) {
		taskLogger->error("ContinuousYouTubeSessionStopFailedUnknownError");
		emit errorOccurred("Unknown error");
	}
}

QCoro::Task<> YouTubeStreamSegmenterWorker::onSegmentSession()
{
	using namespace std::chrono_literals;

	std::shared_ptr<const Logger::ILogger> taskLogger = Logger::NullLogger::instance();

	try {
		taskLogger = std::make_shared<TaskBoundLogger>(
			logger_, "YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask");

		if (stopSource_.stop_requested()) {
			stopSource_ = Jthread::stop_source();
		}
		Jthread::stop_token stoken = stopSource_.get_token();

		taskLogger->info("ContinuousYouTubeSessionSegmenting");

		// --- Initializing scripting context ---

		co_await QCoro::moveToThread(workerThread_);
		// on a worker thread

		EventScriptingContext eventScriptingContext(runtime_, taskLogger, eventHandlerStore_);

		// --- YouTube access token ---
		const std::string accessToken = getAccessToken(stoken, taskLogger, curl_, authStore_);

		// --- Create the next live broadcast ---
		taskLogger->info("YouTubeLiveBroadcastCreatingNext");

		const auto nextLiveBroadcast = createLiveBroadcast(stoken, youTubeApiClient_, accessToken,
								   eventScriptingContext.context,
								   "onCreateYouTubeLiveBroadcastNext",
								   "onSetYouTubeThumbnailNext", taskLogger);

		const std::string nextLiveBroadcastId = nextLiveBroadcast->id.value_or("(ID MISSING)");
		const std::string nextLiveBroadcastTitle =
			(nextLiveBroadcast->snippet && nextLiveBroadcast->snippet->title)
				? *nextLiveBroadcast->snippet->title
				: "(TITLE MISSING)";

		taskLogger->info("YouTubeLiveBroadcastCreatedNext",
				 {{"broadcastId", nextLiveBroadcastId}, {"title", nextLiveBroadcastTitle}});

		// --- Ensure OBS streaming is stopped ---
		if (QThread *mainThread = mainContext_->thread()) {
			co_await QCoro::moveToThread(mainThread);
		} else {
			throw std::runtime_error(
				"MainContextHasNoThreadError(YouTubeStreamSegmenterWorker::onStartSession)");
		}
		// on the main thread

		taskLogger->info("OBSStreamingEnsuringStopped");

		co_await ensureOBSStreamingStopped(stoken, taskLogger);

		taskLogger->info("OBSStreamingEnsuredStopped");

		// --- Start streaming the initial live broadcast ---
		taskLogger->info("StreamingStarting");

		co_await QCoro::sleepFor(5s);

		std::optional<std::string> stoppingLiveStreamId;
		if (currentLiveBroadcast_ && currentLiveBroadcast_->liveStream)
			stoppingLiveStreamId = currentLiveBroadcast_->liveStream->id;

		const auto incomingLiveBroadcast = liveBroadcasts_[1];
		currentLiveBroadcast_ = co_await startStreaming(workerThread_, stoken, youTubeApiClient_, accessToken,
								mainContext_, incomingLiveBroadcast, taskLogger);
		// on a worker thread

		taskLogger->info("StreamingStarted");

		// --- Complete active broadcasts ---
		if (stoppingLiveStreamId) {
			taskLogger->info("YouTubeLiveBroadcastCompletingActive");

			const std::array<std::string, 1> liveStreamIds{*stoppingLiveStreamId};

			completeActiveLiveBroadcasts(stoken, youTubeApiClient_, accessToken, liveStreamIds, taskLogger);

			taskLogger->info("YouTubeLiveBroadcastCompletedActive");

			taskLogger->info("YouTubeLiveStreamDeleting", {{"liveStreamId", *stoppingLiveStreamId}});

			const std::variant<std::shared_ptr<YouTubeApi::YouTubeLiveStream>,
					   std::shared_ptr<YouTubeApi::YouTubeApiError>>
				apiResult =
					youTubeApiClient_->deleteLiveStream(stoken, accessToken, *stoppingLiveStreamId);

			if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult)) {
				const auto &error = std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(apiResult);
				taskLogger->error("YouTubeApiError",
						  {{"code", error->code}, {"message", error->message}});
				throw std::runtime_error(
					"YouTubeApiError(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
			}

			taskLogger->info("YouTubeLiveStreamDeleted", {{"liveStreamId", *stoppingLiveStreamId}});
		} else {
			taskLogger->info("YouTubeLiveStreamSkippingCompleteActiveNoCurrentBroadcast");
		}

		// --- Segment completed ---
		if (!incomingLiveBroadcast->id) {
			taskLogger->error("YouTubeLiveBroadcastIncomingIdMissing");
			throw std::runtime_error(
				"YouTubeLiveBroadcastIncomingIdMissing(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
		}
		if (!incomingLiveBroadcast->snippet || !incomingLiveBroadcast->snippet->title) {
			taskLogger->error("YouTubeLiveBroadcastIncomingTitleMissing");
			throw std::runtime_error(
				"YouTubeLiveBroadcastIncomingTitleMissing(YouTubeStreamSegmenterMainLoop::segmentContinuousSessionTask)");
		}
		taskLogger->info("ContinuousYouTubeSessionSegmented",
				 {{"broadcastId", *incomingLiveBroadcast->id},
				  {"title", *incomingLiveBroadcast->snippet->title}});

		liveBroadcasts_[0] = incomingLiveBroadcast;
		liveBroadcasts_[1] = nextLiveBroadcast;

		emit sessionSegmented();
	} catch (const std::exception &e) {
		taskLogger->error("ContinuousYouTubeSessionSegmentFailed", {{"error", e.what()}});
		QString errStr = e.what();
		emit errorOccurred(errStr);
	} catch (...) {
		taskLogger->error("ContinuousYouTubeSessionSegmentFailedUnknownError");
		emit errorOccurred("Unknown error");
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
