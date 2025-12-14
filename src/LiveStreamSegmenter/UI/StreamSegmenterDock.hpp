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

#include <mutex>

#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <ILogger.hpp>

#include <AuthStore.hpp>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class StreamSegmenterDock : public QWidget {
	Q_OBJECT

public:
	StreamSegmenterDock(std::shared_ptr<const Logger::ILogger> logger, QWidget *parent = nullptr);
	~StreamSegmenterDock() override = default;

	StreamSegmenterDock(const StreamSegmenterDock &) = delete;
	StreamSegmenterDock &operator=(const StreamSegmenterDock &) = delete;
	StreamSegmenterDock(StreamSegmenterDock &&) = delete;
	StreamSegmenterDock &operator=(StreamSegmenterDock &&) = delete;

	void setAuthStore(std::shared_ptr<Store::AuthStore> authStore)
	{
		std::scoped_lock lock(mutex_);
		authStore_ = std::move(authStore);
	}

private slots:
	void onSettingsButtonClicked();

private:
	void setupUi();

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

	std::shared_ptr<Store::AuthStore> authStore_;
	mutable std::mutex mutex_;
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
