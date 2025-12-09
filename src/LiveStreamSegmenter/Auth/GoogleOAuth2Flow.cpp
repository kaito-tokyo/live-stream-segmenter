#include "GoogleOAuth2Flow.hpp"
#include <QTcpSocket>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <string>

// OBS Logger
#include <obs.h>

// libcurl
#include <curl/curl.h>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

// --- Static Helper for Curl ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	auto *str = static_cast<std::string *>(userp);
	str->append(static_cast<char *>(contents), realsize);
	return realsize;
}

// --- Implementation ---

GoogleOAuth2Flow::GoogleOAuth2Flow(QObject *parent) : QObject(parent), server_(new QTcpServer(this))
{
	connect(server_, &QTcpServer::newConnection, this, &GoogleOAuth2Flow::onNewConnection);
}

void GoogleOAuth2Flow::startAuthorization(const QString &clientId, const QString &clientSecret)
{
	clientId_ = clientId;
	clientSecret_ = clientSecret;
	tempState_.clear(); // 状態リセット

	if (!server_->listen(QHostAddress::LocalHost, 0)) {
		emit flowError("Failed to start local server.");
		return;
	}

	quint16 port = server_->serverPort();
	redirectUri_ = QString("http://127.0.0.1:%1/").arg(port);

	QUrl authUrl("https://accounts.google.com/o/oauth2/v2/auth");
	QUrlQuery query;
	query.addQueryItem("client_id", clientId_);
	query.addQueryItem("redirect_uri", redirectUri_);
	query.addQueryItem("response_type", "code");

	// 【重要】チャンネル名を取得するため、YouTube Data API スコープのみを指定
	// email や profile スコープは削除しました
	query.addQueryItem("scope", "https://www.googleapis.com/auth/youtube");

	query.addQueryItem("access_type", "offline");
	query.addQueryItem("prompt", "consent");
	authUrl.setQuery(query);

	QDesktopServices::openUrl(authUrl);

	emit flowStatusChanged(tr("Waiting for browser authorization..."));
}

void GoogleOAuth2Flow::onNewConnection()
{
	QTcpSocket *socket = server_->nextPendingConnection();

	connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
		QByteArray requestData = socket->readAll();
		QString request = QString::fromUtf8(requestData);

		QString code;
		QStringList lines = request.split("\r\n");
		if (!lines.isEmpty()) {
			QString firstLine = lines.first();
			QStringList parts = firstLine.split(" ");
			if (parts.size() >= 2) {
				QUrl url(parts[1]);
				QUrlQuery query(url);
				code = query.queryItemValue("code");
			}
		}

		QString responseBody =
			"<html><body style='text-align:center; font-family:sans-serif; margin-top:50px;'>"
			"<h1 style='color:#4CAF50;'>Authorization Successful!</h1>"
			"<p>You can close this window and return to OBS.</p>"
			"<script>window.close();</script>"
			"</body></html>";

		QString response = QString("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n%1")
					   .arg(responseBody);

		socket->write(response.toUtf8());
		socket->flush();
		socket->disconnectFromHost();
		server_->close();

		if (!code.isEmpty()) {
			emit flowStatusChanged(tr("Exchanging code for token..."));
			exchangeCodeForToken(code);
		} else {
			emit flowError("Authorization failed (No code received).");
		}
	});
}

void GoogleOAuth2Flow::exchangeCodeForToken(const QString &code)
{
	CURL *curl = curl_easy_init();
	if (!curl) {
		emit flowError("Failed to init curl.");
		return;
	}

	std::string readBuffer;
	QUrlQuery postData;
	postData.addQueryItem("code", code);
	postData.addQueryItem("client_id", clientId_);
	postData.addQueryItem("client_secret", clientSecret_);
	postData.addQueryItem("redirect_uri", redirectUri_);
	postData.addQueryItem("grant_type", "authorization_code");

	QByteArray postBytes = postData.toString(QUrl::FullyEncoded).toUtf8();

	curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBytes.constData());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		QString err = QString::fromLatin1(curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		emit flowError(QString("Token exchange failed: %1").arg(err));
		return;
	}
	curl_easy_cleanup(curl);

	QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(readBuffer));
	QJsonObject json = doc.object();

	if (json.contains("error")) {
		emit flowError(QString("API Error: %1").arg(json["error_description"].toString()));
		return;
	}

	// AuthState にトークン情報を反映
	tempState_.updateFromTokenResponse(json);

	if (tempState_.refreshToken().isEmpty()) {
		emit flowError("Failed to get Refresh Token. Please try again.");
		return;
	}

	fetchChannelInfo();
}

void GoogleOAuth2Flow::fetchChannelInfo()
{
	emit flowStatusChanged(tr("Fetching channel info..."));

	CURL *curl = curl_easy_init();
	if (!curl)
		return; // Error handling skipped for brevity

	std::string readBuffer;
	struct curl_slist *headers = NULL;
	QString authHeader = QString("Authorization: Bearer %1").arg(tempState_.accessToken());
	headers = curl_slist_append(headers, authHeader.toUtf8().constData());

	// 【重要】channels API を使用 (mine=true)
	QString url = "https://www.googleapis.com/youtube/v3/channels?part=snippet&mine=true";

	curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		emit flowError("Failed to fetch channel info.");
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(readBuffer));
	QJsonObject json = doc.object();

	QString channelTitle = "Unknown Channel";
	if (json.contains("items")) {
		QJsonArray items = json["items"].toArray();
		if (!items.isEmpty()) {
			channelTitle = items[0].toObject()["snippet"].toObject()["title"].toString();
		}
	}

	// AuthState にチャンネル名をセット (ここでは便宜上 setUserEmail を利用)
	// ※ AuthState側で setChannelTitle 等にリネームしても良い
	tempState_.setUserEmail(channelTitle);

	// 全フロー完了：完成した State を返す
	emit flowSuccess(tempState_);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
