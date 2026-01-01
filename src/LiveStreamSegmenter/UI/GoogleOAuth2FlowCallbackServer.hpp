/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * Live Stream Segmenter - UI Module
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QTcpServer;
class QTcpSocket;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

class GoogleOAuth2FlowCallbackServer : public QObject {
	Q_OBJECT

public:
	explicit GoogleOAuth2FlowCallbackServer(QObject *parent);

	~GoogleOAuth2FlowCallbackServer() noexcept override;

	void listen();

	QUrl getRedirectUri() const;

signals:
	void codeReceived(const QString &code, const QUrl &redirectUrl);

private:
	QTcpServer *const server_;

	void handleRequest(QTcpSocket *socket);

	void sendResponse(QTcpSocket *socket, bool success);
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
