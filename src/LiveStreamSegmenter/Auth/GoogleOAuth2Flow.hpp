#pragma once

#include <QObject>
#include <QTcpServer>
#include "GoogleAuthState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

/**
 * @brief Google OAuth 2.0 のインタラクティブな認証フロー（ブラウザ起動〜コード交換）を担当するクラス
 * * 完了時に GoogleAuthState オブジェクトを生成してシグナルで返します。
 */
class GoogleOAuth2Flow : public QObject {
	Q_OBJECT

public:
	explicit GoogleOAuth2Flow(QObject *parent = nullptr);
	~GoogleOAuth2Flow() override = default;

	/**
     * @brief 認証フローを開始する
     * 1. ローカルサーバー起動 -> 2. ブラウザ起動 -> 3. リダイレクト受信 -> 4. トークン交換
     */
	void startAuthorization(const QString &clientId, const QString &clientSecret);

signals:
	// 成功時: 完成した認証状態オブジェクトを返す
	void flowSuccess(const GoogleAuthState &authState);

	// 失敗時
	void flowError(const QString &message);

	// 進捗状況メッセージ
	void flowStatusChanged(const QString &status);

private slots:
	void onNewConnection();

private:
	// 内部処理
	void exchangeCodeForToken(const QString &code);
	void fetchChannelInfo(); // メールではなくチャンネル名を取得する

	QTcpServer *server_;

	QString clientId_;
	QString clientSecret_;
	QString redirectUri_;

	// フロー構築中の仮状態
	GoogleAuthState tempState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
