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
