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

#include <cstdint>
#include <coroutine>
#include <string>

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QString>
#include <QByteArray>

class AuthCallbackReceiver : public QObject {
	Q_OBJECT
public:
	AuthCallbackReceiver(uint16_t port, std::coroutine_handle<> h, std::string &resultRef)
		: handle_(h),
		  resultRef_(resultRef)
	{
		server_ = new QTcpServer(this);
		connect(server_, &QTcpServer::newConnection, this, &AuthCallbackReceiver::onNewConnection);

		if (!server_->listen(QHostAddress::LocalHost, port)) {
			qWarning() << "Failed to start local auth server on port" << port;
			cleanupAndResume();
		}
	}

private slots:
	void onNewConnection()
	{
		QTcpSocket *socket = server_->nextPendingConnection();
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { handleReadyRead(socket); });
	}

	void handleReadyRead(QTcpSocket *socket)
	{
		QByteArray data = socket->readAll();
		QString request = QString::fromUtf8(data);

		static const QRegularExpression re("^GET\\s+(\\S+)\\s+HTTP");
		QRegularExpressionMatch match = re.match(request);

		if (match.hasMatch()) {
			QString pathAndQuery = match.captured(1);
			QUrl url(pathAndQuery);
			QUrlQuery query(url);

			if (query.hasQueryItem("code")) {
				resultRef_ = query.queryItemValue("code").toStdString();
				sendResponse(socket, true);
			} else {
				sendResponse(socket, false);
			}
		}

		socket->disconnectFromHost();
		cleanupAndResume();
	}

	void sendResponse(QTcpSocket *socket, bool success)
	{
		QString content =
			success ? "<html><body><h1>Login Successful</h1><p>You can close this window now.</p></body></html>"
				: "<html><body><h1>Login Failed</h1><p>Invalid request.</p></body></html>";

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

	void cleanupAndResume()
	{
		if (server_) {
			server_->close();
		}
		if (handle_ && !handle_.done()) {
			handle_.resume();
		}
		deleteLater();
	}

private:
	QTcpServer *server_ = nullptr;
	std::coroutine_handle<> handle_;
	std::string &resultRef_;
};
