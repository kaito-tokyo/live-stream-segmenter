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

#include <mutex>

#include <QWidget>

#include <AuthStore.hpp>
#include <CurlHandle.hpp>
#include <EventHandlerStore.hpp>
#include <ILogger.hpp>
#include <NullLogger.hpp>
#include <ScriptingRuntime.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeStore.hpp>

class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTextEdit;
class QToolButton;
class QVBoxLayout;

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class StreamSegmenterDock : public QWidget {
	Q_OBJECT

	class LoggerAdapter : public Logger::ILogger {
	public:
		explicit LoggerAdapter(StreamSegmenterDock *parent) : parent_(parent) {}

		void log(Logger::LogLevel level, std::string_view name, std::source_location,
			 std::span<const Logger::LogField> context) const noexcept override
		{
			const int logLevel = static_cast<int>(level);
			const QString logName = QString::fromUtf8(name.data(), name.size());

			QMap<QString, QString> contextMap;
			for (const auto &field : context) {
				contextMap.insert(QString::fromUtf8(field.key.data(), field.key.size()),
						  QString::fromUtf8(field.value.data(), field.value.size()));
			}

			QMetaObject::invokeMethod(
				parent_, [=, this]() { parent_->logMessage(logLevel, logName, contextMap); },
				Qt::QueuedConnection);
		}

	private:
		StreamSegmenterDock *const parent_;
	};

public:
	StreamSegmenterDock(std::shared_ptr<Scripting::ScriptingRuntime> runtime, QWidget *parent = nullptr);
	~StreamSegmenterDock() override = default;

	StreamSegmenterDock(const StreamSegmenterDock &) = delete;
	StreamSegmenterDock &operator=(const StreamSegmenterDock &) = delete;
	StreamSegmenterDock(StreamSegmenterDock &&) = delete;
	StreamSegmenterDock &operator=(StreamSegmenterDock &&) = delete;

	std::shared_ptr<const Logger::ILogger> getLoggerAdapter() const { return loggerAdapter_; }

	void setLogger(std::shared_ptr<const Logger::ILogger> logger)
	{
		std::scoped_lock lock(mutex_);
		logger_ = std::move(logger);
	}

	void setAuthStore(std::shared_ptr<Store::AuthStore> authStore)
	{
		std::scoped_lock lock(mutex_);
		authStore_ = std::move(authStore);
	}

	void setEventHandlerStore(std::shared_ptr<Store::EventHandlerStore> eventHandlerStore)
	{
		std::scoped_lock lock(mutex_);
		eventHandlerStore_ = std::move(eventHandlerStore);
	}

	void setYouTubeStore(std::shared_ptr<Store::YouTubeStore> youTubeStore)
	{
		std::scoped_lock lock(mutex_);
		youTubeStore_ = std::move(youTubeStore);
	}

signals:
	void startButtonClicked();
	void stopButtonClicked();
	void segmentNowButtonClicked();

public slots:
	void logMessage(int level, const QString &name, const QMap<QString, QString> &context);

private slots:
	void onSettingsButtonClicked();

private:
	void setupUi();

	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	const std::shared_ptr<const Logger::ILogger> loggerAdapter_;

	// Data Cache
	QString currentStatusText_;
	QString currentStatusColor_;
	QString currentNextTimeText_;
	QString currentTimeRemainingText_;

	// --- UI Components ---

	QVBoxLayout *const mainLayout_;

	// 1. Top Controls
	QHBoxLayout *const topControlLayout_;
	QPushButton *const startButton_;
	QPushButton *const stopButton_;

	// 2. Status Section
	QGroupBox *const statusGroup_;
	QVBoxLayout *const statusLayout_;
	QLabel *const monitorLabel_;

	// 3. Schedule Section
	QGroupBox *const scheduleGroup_;
	QVBoxLayout *const scheduleLayout_;

	QWidget *const currentContainer_;
	QVBoxLayout *const currentLayout_;
	QHBoxLayout *const currentHeader_;
	QHBoxLayout *const currentTitleRow_;
	QLabel *const currentRoleLabel_;
	QLabel *const currentStatusLabel_;
	QLabel *const currentTitleLabel_;
	QToolButton *const currentLinkButton_;

	QWidget *const nextContainer_;
	QVBoxLayout *const nextLayout_;
	QHBoxLayout *const nextHeader_;
	QHBoxLayout *const nextTitleRow_;
	QLabel *const nextRoleLabel_;
	QLabel *const nextStatusLabel_;
	QLabel *const nextTitleLabel_;
	QToolButton *const nextLinkButton_;

	// 4. Log Section
	QGroupBox *const logGroup_;
	QVBoxLayout *const logLayout_;
	QTextEdit *const consoleView_;

	// 5. Bottom Controls
	QVBoxLayout *const bottomControlLayout_;
	QPushButton *const settingsButton_;
	QPushButton *const segmentNowButton_;

	mutable std::mutex mutex_;
	std::shared_ptr<const Logger::ILogger> logger_{Logger::NullLogger::instance()};
	std::shared_ptr<Store::AuthStore> authStore_;
	std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	std::shared_ptr<Store::YouTubeStore> youTubeStore_;
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
