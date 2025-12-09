#include "GoogleAuthState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

bool GoogleAuthState::isAuthorized() const
{
	// リフレッシュトークンさえあれば、いつでもアクセストークンを再取得できるため
	// 「認証済み」とみなします。
	return !refreshToken_.isEmpty();
}

bool GoogleAuthState::isAccessTokenFresh() const
{
	if (accessToken_.isEmpty() || !tokenExpiration_.isValid()) {
		return false;
	}

	// 安全マージン: 期限切れの60秒前には「もう使えない」と判断してリフレッシュを促す
	const qint64 kExpirationMarginSeconds = 60;

	QDateTime now = QDateTime::currentDateTimeUtc();
	return now.addSecs(kExpirationMarginSeconds) < tokenExpiration_;
}

void GoogleAuthState::updateFromTokenResponse(const QJsonObject &json)
{
	// 1. アクセストークン (毎回変わる)
	if (json.contains("access_token")) {
		accessToken_ = json["access_token"].toString();
	}

	// 2. リフレッシュトークン
	// 初回認証時は返ってくるが、アクセストークン更新(Refresh)時は
	// 返ってこないことが多い。空の場合は既存のものを消さないようにする。
	if (json.contains("refresh_token")) {
		QString newRefresh = json["refresh_token"].toString();
		if (!newRefresh.isEmpty()) {
			refreshToken_ = newRefresh;
		}
	}

	// 3. 有効期限 (expires_in は "秒数" で返ってくる)
	if (json.contains("expires_in")) {
		int expiresInSeconds = json["expires_in"].toInt();
		// 現在時刻 + 秒数 = 絶対時刻 (UTCで計算・保持する)
		tokenExpiration_ = QDateTime::currentDateTimeUtc().addSecs(expiresInSeconds);
	}

	// 4. スコープ (権限が変わった場合など)
	if (json.contains("scope")) {
		scope_ = json["scope"].toString();
	}
}

void GoogleAuthState::setUserEmail(const QString &email)
{
	email_ = email;
}

void GoogleAuthState::clear()
{
	accessToken_.clear();
	refreshToken_.clear();
	email_.clear();
	scope_.clear();
	tokenExpiration_ = QDateTime();
}

QJsonObject GoogleAuthState::toJson() const
{
	QJsonObject json;

	// 必須保存項目
	json["refresh_token"] = refreshToken_;
	json["email"] = email_;

	// キャッシュとしての保存項目
	// (アプリ再起動後も、期限内ならAPIを叩かずに即座に使えるようにするため)
	json["access_token"] = accessToken_;
	if (tokenExpiration_.isValid()) {
		json["expires_at"] = tokenExpiration_.toString(Qt::ISODate);
	}

	if (!scope_.isEmpty()) {
		json["scope"] = scope_;
	}

	return json;
}

GoogleAuthState GoogleAuthState::fromJson(const QJsonObject &json)
{
	GoogleAuthState state;

	state.refreshToken_ = json["refresh_token"].toString();
	state.email_ = json["email"].toString();
	state.accessToken_ = json["access_token"].toString();

	if (json.contains("expires_at")) {
		state.tokenExpiration_ = QDateTime::fromString(json["expires_at"].toString(), Qt::ISODate);
	}

	if (json.contains("scope")) {
		state.scope_ = json["scope"].toString();
	}

	return state;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
