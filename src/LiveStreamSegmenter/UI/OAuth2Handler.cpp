#include "OAuth2Handler.hpp"
#include <QTcpSocket>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

OAuth2Handler::OAuth2Handler(QObject *parent)
	: QObject(parent),
	  server_(new QTcpServer(this)),
	  networkManager_(new QNetworkAccessManager(this))
{
	// サーバーへの接続検知
	connect(server_, &QTcpServer::newConnection, this, &OAuth2Handler::onNewConnection);
}

void OAuth2Handler::startAuthorization(const QString &clientId, const QString &clientSecret)
{
	clientId_ = clientId;
	clientSecret_ = clientSecret;

	// 1. ローカルサーバー起動 (ポート0 = 空きポート自動割り当て)
	if (!server_->listen(QHostAddress::LocalHost, 0)) {
		emit authError("Failed to start local server.");
		return;
	}

	quint16 port = server_->serverPort();
	redirectUri_ = QString("http://127.0.0.1:%1/").arg(port);

	// 2. ブラウザ起動用URL作成
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

		// Code抽出 (簡易パーサ)
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

		// ブラウザへのレスポンス返却
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

		server_->close(); // サーバー停止 (ポート解放)

		if (!code.isEmpty()) {
			emit statusChanged(tr("Exchanging code for token..."));

			// トークン交換リクエスト
			QUrl tokenUrl("https://oauth2.googleapis.com/token");
			QNetworkRequest req(tokenUrl);
			req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

			QUrlQuery postData;
			postData.addQueryItem("code", code);
			postData.addQueryItem("client_id", clientId_);
			postData.addQueryItem("client_secret", clientSecret_);
			postData.addQueryItem("redirect_uri", redirectUri_);
			postData.addQueryItem("grant_type", "authorization_code");

			QNetworkReply *reply =
				networkManager_->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
			connect(reply, &QNetworkReply::finished, this,
				[this, reply]() { onTokenExchangeFinished(reply); });
		} else {
			emit authError("Authorization failed (No code received).");
		}
	});
}

void OAuth2Handler::onTokenExchangeFinished(QNetworkReply *reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		emit authError(QString("Token Error: %1").arg(reply->errorString()));
		return;
	}

	QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
	tempAccessToken_ = json["access_token"].toString();
	tempRefreshToken_ = json["refresh_token"].toString();

	if (tempRefreshToken_.isEmpty()) {
		emit authError("Failed to get Refresh Token. Revoke app permissions and try again.");
		return;
	}

	// ユーザー情報取得へ
	emit statusChanged(tr("Fetching user info..."));

	QUrl userUrl("https://www.googleapis.com/oauth2/v2/userinfo");
	QNetworkRequest req(userUrl);
	req.setRawHeader("Authorization", QString("Bearer %1").arg(tempAccessToken_).toUtf8());

	QNetworkReply *userReply = networkManager_->get(req);
	connect(userReply, &QNetworkReply::finished, this, [this, userReply]() { onUserInfoFinished(userReply); });
}

void OAuth2Handler::onUserInfoFinished(QNetworkReply *reply)
{
	reply->deleteLater();

	QString email = "Unknown";
	if (reply->error() == QNetworkReply::NoError) {
		email = QJsonDocument::fromJson(reply->readAll()).object()["email"].toString();
	}

	// 全フロー完了
	emit authSuccess(tempRefreshToken_, tempAccessToken_, email);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
