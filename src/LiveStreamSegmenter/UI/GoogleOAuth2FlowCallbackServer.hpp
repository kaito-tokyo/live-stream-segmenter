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

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

#ifndef Q_MOC_RUN
#include <coroutine>
#endif

#include <QByteArray>
#include <QDebug>
#include <QHostAddress>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

class GoogleOAuth2FlowCallbackServer : public QObject {
	Q_OBJECT
public:
	explicit GoogleOAuth2FlowCallbackServer(QObject *parent = nullptr) : QObject(parent)
	{
		server_ = new QTcpServer(this);
		if (!server_->listen(QHostAddress::LocalHost, 0)) {
			throw std::runtime_error(fmt::format("InitError(GoogleOAuth2FlowCallbackServer):{}", server_->errorString().toStdString()));
		}
	}

	~GoogleOAuth2FlowCallbackServer() override
	{
		if (server_) {
			server_->close();
		}
	}

	std::uint16_t serverPort() const { return server_->serverPort(); }

	struct CodeAwaiterState {
		std::string result;
		std::atomic<bool> isAlive{true};
		std::atomic<bool> resumed{false};
	};

	struct CodeAwaiter {
		GoogleOAuth2FlowCallbackServer *server;
		QMetaObject::Connection conn;

		explicit CodeAwaiter(GoogleOAuth2FlowCallbackServer *s) : server(s), state(std::make_shared<CodeAwaiterState>()) {}

		~CodeAwaiter()
		{
			if (conn) {
				QObject::disconnect(conn);
			}
		}

		std::shared_ptr<CodeAwaiterState> state;

		bool await_ready() { return false; }

		void await_suspend(std::coroutine_handle<> h)
		{
			auto statePtr = state;
			QPointer<GoogleOAuth2FlowCallbackServer> safeServer = server;

			conn = QObject::connect(
				server->server_, &QTcpServer::newConnection, safeServer, [safeServer, h, statePtr]() {
					if (!safeServer)
						return;

					QTcpSocket *socket = safeServer->server_->nextPendingConnection();

					QObject::connect(socket, &QTcpSocket::readyRead, safeServer,
							 [safeServer, socket, h, statePtr]() {
								 if (!statePtr->isAlive)
									 return;
								 if (safeServer) {
									 safeServer->handleRequest(socket, statePtr, h);
								 }
							 });
				});
		}

		std::string await_resume() { return state->result; }
	};

	CodeAwaiter waitForCode() { return CodeAwaiter{this}; }

private:
	QTcpServer *server_ = nullptr;

	void handleRequest(QTcpSocket *socket, std::shared_ptr<CodeAwaiterState> statePtr, std::coroutine_handle<> h)
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
				statePtr->result = query.queryItemValue("code").toStdString();
				success = true;
			}
		}

		sendResponse(socket, success);

		connect(socket, &QTcpSocket::disconnected, socket, [socket, h, statePtr]() {
			socket->deleteLater();

			if (statePtr->isAlive && !statePtr->resumed.exchange(true)) {
				h.resume();
			}
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

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
