#pragma once

#include <QObject>
#include <QTcpServer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

class OAuth2Handler : public QObject {
	Q_OBJECT

public:
	explicit OAuth2Handler(QObject *parent = nullptr);
	~OAuth2Handler() override = default;

	// 認証フローを開始
	void startAuthorization(const QString &clientId, const QString &clientSecret);

signals:
	// 認証成功: トークンとメールアドレスを返す
	void authSuccess(const QString &refreshToken, const QString &accessToken, const QString &email);

	// 失敗・エラー
	void authError(const QString &message);

	// ステータス更新（UIに「今なにをしてるか」を表示させるため）
	void statusChanged(const QString &status);

private slots:
	void onNewConnection();
	void onTokenExchangeFinished(QNetworkReply *reply);
	void onUserInfoFinished(QNetworkReply *reply);

private:
	QTcpServer *server_;
	QNetworkAccessManager *networkManager_;

	QString clientId_;
	QString clientSecret_;
	QString redirectUri_;

	// 一時データ
	QString tempRefreshToken_;
	QString tempAccessToken_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
