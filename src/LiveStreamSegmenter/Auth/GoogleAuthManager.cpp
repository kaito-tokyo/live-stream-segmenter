#include "GoogleAuthManager.hpp"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-module.h>
#include <obs.h> // ログ用

// libcurl
#include <curl/curl.h>
#include <string>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

// --- Static Helper ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	auto *str = static_cast<std::string *>(userp);
	str->append(static_cast<char *>(contents), realsize);
	return realsize;
}

// --- Implementation ---

GoogleAuthManager::GoogleAuthManager(QObject *parent) : QObject(parent), flow_(new GoogleOAuth2Flow(this))
{
	// Flowからのシグナル接続
	connect(flow_, &GoogleOAuth2Flow::flowSuccess, this, &GoogleAuthManager::onFlowSuccess);
	connect(flow_, &GoogleOAuth2Flow::flowError, this, &GoogleAuthManager::onFlowError);
	connect(flow_, &GoogleOAuth2Flow::flowStatusChanged, this, &GoogleAuthManager::onFlowStatusChanged);

	// 起動時にディスクからロード
	loadTokenFromDisk();
}

void GoogleAuthManager::setCredentials(const QString &clientId, const QString &clientSecret)
{
	clientId_ = clientId;
	clientSecret_ = clientSecret;
}

bool GoogleAuthManager::isAuthenticated() const
{
	return state_.isAuthorized();
}

QString GoogleAuthManager::currentChannelName() const
{
	return state_.email(); // AuthState側で email_ にチャンネル名を入れている前提
}

void GoogleAuthManager::startLogin()
{
	if (clientId_.isEmpty() || clientSecret_.isEmpty()) {
		emit loginError("Client ID/Secret not set.");
		return;
	}
	flow_->startAuthorization(clientId_, clientSecret_);
}

void GoogleAuthManager::logout()
{
	state_.clear();
	saveTokenToDisk(); // 空の状態を保存＝削除
	emit authStateChanged();
}

// ==========================================
//  Token Vending (核心機能)
// ==========================================

void GoogleAuthManager::getAccessToken(std::function<void(QString)> onSuccess, std::function<void(QString)> onError)
{
	if (!isAuthenticated()) {
		onError("Not logged in (No Refresh Token).");
		return;
	}

	// 1. トークンがまだ新鮮なら、そのまま返す
	if (state_.isAccessTokenFresh()) {
		onSuccess(state_.accessToken());
		return;
	}

	// 2. 腐っているならリフレッシュする
	if (clientId_.isEmpty() || clientSecret_.isEmpty()) {
		onError("Cannot refresh: Client ID/Secret missing.");
		return;
	}

	blog(LOG_INFO, "[GoogleAuthManager] Access token expired. Refreshing...");

	// リフレッシュ実行
	performRefresh(onSuccess, onError);
}

void GoogleAuthManager::performRefresh(std::function<void(QString)> onSuccess, std::function<void(QString)> onError)
{
	CURL *curl = curl_easy_init();
	if (!curl) {
		onError("Failed to init curl.");
		return;
	}

	std::string readBuffer;

	// リフレッシュ用パラメータ
	// grant_type=refresh_token
	QString postDataStr =
		QString("client_id=%1&client_secret=%2&refresh_token=%3&grant_type=refresh_token")
			.arg(QUrl::toPercentEncoding(clientId_), QUrl::toPercentEncoding(clientSecret_),
			     QUrl::toPercentEncoding(state_.refreshToken())); // 保持しているRefreshTokenを使う

	QByteArray postBytes = postDataStr.toUtf8();

	curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBytes.constData());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		QString err = QString::fromLatin1(curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		blog(LOG_ERROR, "[GoogleAuthManager] Refresh failed: %s", err.toUtf8().constData());
		onError(QString("Refresh Network Error: %1").arg(err));
		return;
	}
	curl_easy_cleanup(curl);

	// JSONパース
	QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(readBuffer));
	QJsonObject json = doc.object();

	if (json.contains("error")) {
		QString errMsg = json["error_description"].toString();
		blog(LOG_ERROR, "[GoogleAuthManager] Refresh API Error: %s", errMsg.toUtf8().constData());

		// リフレッシュトークンが無効になった場合（Revokeされた等）
		if (json["error"].toString() == "invalid_grant") {
			logout(); // 強制ログアウト
			onError("Session expired (Revoked). Please login again.");
		} else {
			onError(QString("Refresh API Error: %1").arg(errMsg));
		}
		return;
	}

	// 成功: Stateを更新して保存
	state_.updateFromTokenResponse(json);
	saveTokenToDisk();

	blog(LOG_INFO, "[GoogleAuthManager] Token refreshed successfully.");
	emit authStateChanged(); // 有効期限が変わったので通知

	// 待っていた呼び出し元に新しいトークンを渡す
	onSuccess(state_.accessToken());
}

// ==========================================
//  Flow Slots (UIからの操作結果)
// ==========================================

void GoogleAuthManager::onFlowSuccess(const GoogleAuthState &newState)
{
	state_ = newState;
	saveTokenToDisk();
	emit authStateChanged();
}

void GoogleAuthManager::onFlowError(const QString &message)
{
	emit loginError(message);
}

void GoogleAuthManager::onFlowStatusChanged(const QString &status)
{
	emit loginStatusChanged(status);
}

// ==========================================
//  Persistence (保存/読み込み)
// ==========================================

QString GoogleAuthManager::getTokenFilePath() const
{
	char *path = obs_module_get_config_path(obs_current_module(), "token.json");
	if (!path)
		return QString();
	QString qPath = QString::fromUtf8(path);
	bfree(path);
	return qPath;
}

void GoogleAuthManager::saveTokenToDisk()
{
	QString path = getTokenFilePath();
	if (path.isEmpty())
		return;

	QFile file(path);
	if (file.open(QIODevice::WriteOnly)) {
		QJsonDocument doc(state_.toJson());
		file.write(doc.toJson());
		file.close();
	}
}

void GoogleAuthManager::loadTokenFromDisk()
{
	QString path = getTokenFilePath();
	QFile file(path);
	if (file.open(QIODevice::ReadOnly)) {
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		state_ = GoogleAuthState::fromJson(doc.object());

		if (state_.isAuthorized()) {
			emit authStateChanged();
		}
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
