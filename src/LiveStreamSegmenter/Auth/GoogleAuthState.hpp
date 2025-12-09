#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

/**
 * @brief Google OAuth 2.0 のトークン情報と認証状態を保持するデータクラス
 * * トークンの有効期限判定や、Google APIレスポンス(JSON)からの更新ロジック、
 * およびローカルファイルへの保存/復元(シリアライズ)機能を提供します。
 */
class GoogleAuthState {
public:
	GoogleAuthState() = default;
	~GoogleAuthState() = default;

	// --- 状態判定メソッド ---

	/**
     * @brief 永続的な認証情報（リフレッシュトークン）を持っているか
     * @return true: ログイン済みとみなせる / false: 未ログイン
     */
	bool isAuthorized() const;

	/**
     * @brief 現在のアクセストークンが有効（期限切れしていない）か
     * * 通信レイテンシ等を考慮し、実際の期限より60秒早めに「期限切れ」と判定します。
     * @return true: そのままAPIコールに使用可能 / false: リフレッシュが必要
     */
	bool isAccessTokenFresh() const;

	// --- データ操作メソッド ---

	/**
     * @brief Token Endpoint からのレスポンスJSONを取り込んで状態を更新する
     * * Googleから返却される {"access_token": "...", "expires_in": 3600, ...} を解析します。
     * リフレッシュトークンは含まれている場合のみ更新します。
     */
	void updateFromTokenResponse(const QJsonObject &googleResponse);

	/**
     * @brief ユーザーの識別情報（メールアドレス）をセットする
     * UserInfo API の結果から設定します。
     */
	void setUserEmail(const QString &email);

	/**
     * @brief データをクリアする（ログアウト時など）
     */
	void clear();

	// --- 永続化 (Save/Load) ---

	/**
     * @brief 保存用JSONを生成 (token.json用)
     */
	QJsonObject toJson() const;

	/**
     * @brief 保存済みJSONからインスタンスを復元
     */
	static GoogleAuthState fromJson(const QJsonObject &json);

	// --- Getters ---
	QString accessToken() const { return accessToken_; }
	QString refreshToken() const { return refreshToken_; }
	QString email() const { return email_; }
	QDateTime expirationDate() const { return tokenExpiration_; }

private:
	QString accessToken_;
	QString refreshToken_;
	QString email_;

	// アクセストークンの有効期限（絶対時刻・UTC）
	QDateTime tokenExpiration_;

	// スコープ文字列（必要に応じて保持）
	QString scope_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
