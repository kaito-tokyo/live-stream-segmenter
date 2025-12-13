/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file 
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#include "OAuth2Handler.hpp"
#include <QTcpSocket>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <string>

#include <obs.h>

// libcurl
#include <curl/curl.h>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

// --- Static Helper for Curl Callback ---
// レスポンスデータを std::string に追記するコールバック
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	auto *str = static_cast<std::string *>(userp);
	str->append(static_cast<char *>(contents), realsize);
	return realsize;
}

// --- Implementation ---

OAuth2Handler::OAuth2Handler(QObject *parent) : QObject(parent), server_(new QTcpServer(this))
{
	connect(server_, &QTcpServer::newConnection, this, &OAuth2Handler::onNewConnection);
}

void OAuth2Handler::startAuthorization(const QString &clientId, const QString &clientSecret)
{
	clientId_ = clientId;
	clientSecret_ = clientSecret;

	// HTTPサーバー起動 (SSL不要)
	if (!server_->listen(QHostAddress::LocalHost, 0)) {
		emit authError("Failed to start local server.");
		return;
	}

	quint16 port = server_->serverPort();
	redirectUri_ = QString("http://127.0.0.1:%1/").arg(port);

	QUrl authUrl("https://accounts.google.com/o/oauth2/v2/auth");
	QUrlQuery query;
	query.addQueryItem("client_id", clientId_);
	query.addQueryItem("redirect_uri", redirectUri_);
	query.addQueryItem("response_type", "code");
	query.addQueryItem("scope", "https://www.googleapis.com/auth/youtube");
	query.addQueryItem("access_type", "offline");
	query.addQueryItem("prompt", "consent");
	authUrl.setQuery(query);

	QDesktopServices::openUrl(authUrl);

	emit statusChanged(tr("Waiting for browser authorization..."));
}

void OAuth2Handler::onNewConnection()
{
	QTcpSocket *socket = server_->nextPendingConnection();

	connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
		QByteArray requestData = socket->readAll();
		QString request = QString::fromUtf8(requestData);

		// Code抽出
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

		// ブラウザへの応答
		QString responseBody =
			"<html><body style='text-align:center; font-family:sans-serif; margin-top:50px;'>"
			"<h1 style='color:#4CAF50;'>Authorization Successful!</h1>"
			"<p>You can close this window.</p>"
			"<script>window.close();</script>"
			"</body></html>";

		QString response = QString("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n%1")
					   .arg(responseBody);

		socket->write(response.toUtf8());
		socket->flush();
		socket->disconnectFromHost();

		server_->close();

		if (!code.isEmpty()) {
			emit statusChanged(tr("Exchanging code for token..."));
			// ここで libcurl ロジックへ
			exchangeCode(code);
		} else {
			emit authError("Authorization failed (No code received).");
		}
	});
}

// libcurl: Token Exchange
void OAuth2Handler::exchangeCode(const QString &code)
{
	// ... (curl初期化等は変更なし) ...
	CURL *curl = curl_easy_init();
	if (!curl) {
		emit authError("Failed to init curl.");
		return;
	}

	std::string readBuffer;
	// ... (パラメータ設定等は変更なし) ...

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

	// 【デバッグ用】SSL検証無効化（もしSSLエラーが疑われる場合のみ有効化してください）
	// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		QString err = QString::fromLatin1(curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		// 【ログ】Curlレベルのエラーを出力
		blog(LOG_ERROR, "[OAuth2] Token Exchange Curl Error: %s", err.toUtf8().constData());
		emit authError(QString("Curl error: %1").arg(err));
		return;
	}
	curl_easy_cleanup(curl);

	// 【ログ】生のレスポンスデータを全て出力 (重要)
	blog(LOG_INFO, "[OAuth2] Token Response Raw: %s", readBuffer.c_str());

	QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(readBuffer));
	QJsonObject json = doc.object();

	// エラーレスポンスのチェック
	if (json.contains("error")) {
		QString errDesc = json["error_description"].toString();
		blog(LOG_ERROR, "[OAuth2] API Error: %s", errDesc.toUtf8().constData());
	}

	tempAccessToken_ = json["access_token"].toString();
	tempRefreshToken_ = json["refresh_token"].toString();

	blog(LOG_INFO, "[OAuth2] Access Token length: %d", (int)tempAccessToken_.length());
	blog(LOG_INFO, "[OAuth2] Refresh Token length: %d", (int)tempRefreshToken_.length());

	if (tempRefreshToken_.isEmpty()) {
		emit authError("Failed to get Refresh Token. Check logs.");
		return;
	}

	fetchUserInfo();
}

// libcurl: User Info
void OAuth2Handler::fetchUserInfo()
{
	emit statusChanged(tr("Fetching user info..."));

	CURL *curl = curl_easy_init();
	if (!curl) {
		emit authError("Failed to init curl.");
		return;
	}

	std::string readBuffer;
	struct curl_slist *headers = NULL;
	QString authHeader = QString("Authorization: Bearer %1").arg(tempAccessToken_);
	headers = curl_slist_append(headers, authHeader.toUtf8().constData());

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/oauth2/v2/userinfo");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		QString err = QString::fromLatin1(curl_easy_strerror(res));
		// 【ログ】Curlエラー
		blog(LOG_ERROR, "[OAuth2] User Info Curl Error: %s", err.toUtf8().constData());
		emit authError(QString("Curl error: %1").arg(err));
		return;
	}

	// 【ログ】生のレスポンスデータを全て出力 (重要)
	// ここで "Unknown" の原因がわかります（例: 権限不足エラーなど）
	blog(LOG_INFO, "[OAuth2] User Info Response Raw: %s", readBuffer.c_str());

	QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(readBuffer));
	QJsonObject obj = doc.object();

	// emailフィールドがあるか確認
	if (!obj.contains("email")) {
		blog(LOG_WARNING, "[OAuth2] 'email' field missing in response.");
	}

	QString email = obj["email"].toString();
	if (email.isEmpty())
		email = "Unknown";

	emit authSuccess(tempRefreshToken_, tempAccessToken_, email);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
