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

#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <ILogger.hpp>

#include <AuthService.hpp>
#include <GoogleAuthManager.hpp>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class StreamSegmenterDock : public QWidget, public Logger::ILogger {
	Q_OBJECT

public:
	StreamSegmenterDock(std::shared_ptr<Service::AuthService> authService, std::shared_ptr<const Logger::ILogger> logger, QWidget *parent = nullptr);
	~StreamSegmenterDock() override = default;

	void setSystemStatus(const QString &statusText, const QString &colorCode);
	void setNextSegmentationTime(const QDateTime &time);
	void setTimeRemaining(int remainingSeconds);

	void updateCurrentStream(const QString &title, const QString &status, const QString &url = "");
	void updateNextStream(const QString &title, const QString &status, const QString &url = "");

protected:
	void log(LogLevel level, std::string_view message) const noexcept override;
	const char *getPrefix() const noexcept override { return ""; }

signals:
	void logRequest(int level, const QString &message);

private slots:
	void onLogRequest(int level, const QString &message);
	void onStartClicked();
	void onStopClicked();
	void onSegmentNowClicked();
	void onSettingsClicked();
	void onLinkButtonClicked();

private:
	void setupUi();
	void updateMonitorLabel();
	
	std::shared_ptr<Service::AuthService> authService_;
	const std::shared_ptr<const Logger::ILogger> logger_;

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

	std::shared_ptr<Auth::GoogleAuthManager> googleAuthManager_ = nullptr;
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
