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

#include <coroutine>
#include <cstdint>
#include <optional>
#include <string>

#include <QByteArray>
#include <QDebug>
#include <QHostAddress>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

class GoogleOAuth2FlowCallbackServer : public QObject {
	Q_OBJECT
public:
	explicit GoogleOAuth2FlowCallbackServer(QObject *parent = nullptr) : QObject(parent)
	{
		server_ = new QTcpServer(this);
		if (!server_->listen(QHostAddress::LocalHost, 0)) {
			qWarning() << "Failed to start local auth server";
		}
	}

	~GoogleOAuth2FlowCallbackServer() override
	{
		if (server_) {
			server_->close();
		}
	}

	std::uint16_t serverPort() const { return server_->serverPort(); }

	struct CodeAwaiter {
		GoogleOAuth2FlowCallbackServer *server;
		std::string result;
		QMetaObject::Connection conn;

		bool await_ready() { return false; }

		void await_suspend(std::coroutine_handle<> h)
		{
			conn = QObject::connect(server->server_, &QTcpServer::newConnection, server, [this, h]() {
				QTcpSocket *socket = server->server_->nextPendingConnection();
				QObject::connect(socket, &QTcpSocket::readyRead, server,
						 [this, socket, h]() { server->handleRequest(socket, result, h); });
			});
		}

		std::string await_resume() { return result; }

		~CodeAwaiter()
		{
			if (conn) {
				QObject::disconnect(conn);
			}
		}
	};

	CodeAwaiter waitForCode() { return CodeAwaiter{this, ""}; }

private:
	QTcpServer *server_ = nullptr;

	void handleRequest(QTcpSocket *socket, std::string &resultOut, std::coroutine_handle<> h)
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
				resultOut = query.queryItemValue("code").toStdString();
				success = true;
			}
		}

		sendResponse(socket, success);

		connect(socket, &QTcpSocket::disconnected, [socket, h]() {
			socket->deleteLater();
			if (!h.done())
				h.resume();
		});

		socket->disconnectFromHost();
	}

	void sendResponse(QTcpSocket *socket, bool success)
	{
		QString content = success ? "<h1>Login Successful</h1><p>You can close this window now.</p>"
					  : "<h1>Login Failed</h1><p>Invalid request.</p>";

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
};
