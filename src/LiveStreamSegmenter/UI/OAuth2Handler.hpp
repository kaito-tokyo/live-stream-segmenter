#pragma once

#include <QObject>
#include <QTcpServer>

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

	// ステータス更新
	void statusChanged(const QString &status);

private slots:
	void onNewConnection();

private:
	// 内部実装用の private メソッド
	void exchangeCode(const QString &code);
	void fetchUserInfo();

	QTcpServer *server_;

	QString clientId_;
	QString clientSecret_;
	QString redirectUri_;

	// 一時データ
	QString tempRefreshToken_;
	QString tempAccessToken_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
