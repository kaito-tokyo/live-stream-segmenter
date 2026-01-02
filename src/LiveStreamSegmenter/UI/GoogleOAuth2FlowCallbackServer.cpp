/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#include "GoogleOAuth2FlowCallbackServer.hpp"

#include <QByteArray>
#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

GoogleOAuth2FlowCallbackServer::GoogleOAuth2FlowCallbackServer(QObject *parent)
	: QObject(parent),
	  server_(new QTcpServer(this))
{
}

GoogleOAuth2FlowCallbackServer::~GoogleOAuth2FlowCallbackServer() noexcept
{
	if (server_) {
		try {
			server_->close();
		} catch (...) {
			// ignore
		}
	}
}

void GoogleOAuth2FlowCallbackServer::listen()
{
	connect(server_, &QTcpServer::newConnection, this, [this]() {
		QTcpSocket *socket = server_->nextPendingConnection();
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { handleRequest(socket); });
	});

	if (!server_->listen(QHostAddress::LocalHost, 0)) {
		throw std::runtime_error("ListenError(listen)");
	}
}

QUrl GoogleOAuth2FlowCallbackServer::getRedirectUri() const
{
	quint16 port = server_->serverPort();
	if (port == 0) {
		throw std::runtime_error("ServerNotListeningError(getRedirectUri)");
	} else {
		QUrl url;
		url.setScheme("http");
		url.setHost("localhost");
		url.setPort(static_cast<int>(port));
		url.setPath("/callback");
		return url;
	}
}

void GoogleOAuth2FlowCallbackServer::handleRequest(QTcpSocket *socket)
{
	QByteArray data = socket->readAll();
	QString request = QString::fromUtf8(data);

	static const QRegularExpression re("^GET\\s+(\\S+)\\s+HTTP");
	QRegularExpressionMatch match = re.match(request);

	bool success = false;

	if (match.hasMatch()) {
		QString path = match.captured(1);

		QUrl url("http://localhost" + path);
		QUrlQuery query(url);

		if (query.hasQueryItem("code")) {
			QString code = query.queryItemValue("code");
			QUrl redirectUri = getRedirectUri();
			success = true;
			emit codeReceived(code, redirectUri);
		}
	}

	sendResponse(socket, success);

	connect(socket, &QTcpSocket::disconnected, socket, [socket]() { socket->deleteLater(); });

	socket->disconnectFromHost();
}

void GoogleOAuth2FlowCallbackServer::sendResponse(QTcpSocket *socket, bool success)
{
	QString content = success ? tr("<h1>Login Successful</h1><p>You can close this window now.</p>")
				  : tr("<h1>Login Failed</h1><p>Invalid request.</p>");

	QString response = QString("HTTP/1.1 200 OK\r\n"
				   "Content-Type: text/html; charset=utf-8\r\n"
				   "Content-Length: %1\r\n"
				   "Connection: close\r\n"
				   "\r\n"
				   "%2")
				   .arg(content.toUtf8().size())
				   .arg(content);

	socket->write(response.toUtf8());
	socket->flush();
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
