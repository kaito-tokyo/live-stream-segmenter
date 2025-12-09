#pragma once

#include <QObject>
#include <functional>
#include "GoogleAuthState.hpp"
#include "GoogleOAuth2Flow.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

/**
 * @brief 認証機能の統括マネージャー
 * * トークンの永続化(保存/読み込み)、有効期限管理、自動リフレッシュ、
 * * およびインタラクティブなログインフローの制御を行います。
 */
class GoogleAuthManager : public QObject {
	Q_OBJECT

public:
	explicit GoogleAuthManager(QObject *parent = nullptr);
	~GoogleAuthManager() override = default;

	// --- 設定 ---

	/**
     * @brief クライアントID/Secretを設定する
     * * リフレッシュ処理に必要になるため、起動時や設定変更時に必ず呼んでください。
     */
	void setCredentials(const QString &clientId, const QString &clientSecret);

	// --- 状態確認 ---

	bool isAuthenticated() const;
	QString currentChannelName() const;

	// --- 操作 ---

	/**
     * @brief ログインフローを開始する (UI用)
     * * ブラウザを開いてユーザーに認証させます。
     */
	void startLogin();

	/**
     * @brief ログアウト (トークン破棄)
     */
	void logout();

	// --- トークン供給 (Core Logic用) ---

	/**
     * @brief 有効なアクセストークンを取得する (非同期)
     * * トークンが有効なら即座に onSuccess を呼びます。
     * * 期限切れなら、内部でリフレッシュ処理を行ってから onSuccess を呼びます。
     * * 失敗した場合は onError を呼びます。
     */
	void getAccessToken(std::function<void(QString token)> onSuccess, std::function<void(QString error)> onError);

signals:
	// ログイン状態やチャンネル名が変わった時に発火
	void authStateChanged();

	// ログインフロー中のステータス（"ブラウザ待機中..."など）
	void loginStatusChanged(const QString &status);

	// ログインフロー失敗
	void loginError(const QString &message);

private slots:
	void onFlowSuccess(const GoogleAuthState &newState);
	void onFlowError(const QString &message);
	void onFlowStatusChanged(const QString &status);

private:
	// 内部ヘルパー
	void loadTokenFromDisk();
	void saveTokenToDisk();
	QString getTokenFilePath() const;

	// リフレッシュ処理 (libcurl使用)
	void performRefresh(std::function<void(QString token)> onSuccess, std::function<void(QString error)> onError);

	GoogleAuthState state_;
	GoogleOAuth2Flow *flow_;

	// リフレッシュに必要な情報
	QString clientId_;
	QString clientSecret_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
