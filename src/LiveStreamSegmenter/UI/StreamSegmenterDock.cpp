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

#include "StreamSegmenterDock.hpp"

#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QMetaObject>
#include <QScrollBar>
#include <QUrl>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

#include <fmt/format.h>

#include <MultiLogger.hpp>

#include "SettingsDialog.hpp"

using namespace KaitoTokyo::Logger;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

StreamSegmenterDock::StreamSegmenterDock(std::shared_ptr<Scripting::ScriptingRuntime> runtime, QWidget *parent)
	: QWidget(parent),
	  runtime_(runtime ? std::move(runtime)
			   : throw std::invalid_argument(
				     "RuntimeIsNullError(StreamSegmenterDock::StreamSegmenterDock)")),
	  loggerAdapter_(std::make_shared<const LoggerAdapter>(this)),
	  mainLayout_(new QVBoxLayout(this)),

	  // Top Controls
	  topControlLayout_(new QHBoxLayout(this)),
	  startButton_(new QPushButton(tr("Start"), this)),
	  stopButton_(new QPushButton(tr("Stop"), this)),

	  // Status Monitor
	  statusGroup_(new QGroupBox(tr("System Monitor"), this)),
	  statusLayout_(new QVBoxLayout(statusGroup_)),
	  monitorLabel_(new QLabel(statusGroup_)),

	  // Schedule
	  scheduleGroup_(new QGroupBox(tr("Broadcast Schedule"), this)),
	  scheduleLayout_(new QVBoxLayout(scheduleGroup_)),

	  currentContainer_(new QWidget(scheduleGroup_)),
	  currentLayout_(new QVBoxLayout(currentContainer_)),
	  currentHeader_(new QHBoxLayout(currentContainer_)),
	  currentTitleRow_(new QHBoxLayout(currentContainer_)),
	  currentRoleLabel_(new QLabel(tr("CURRENT"), currentContainer_)),
	  currentStatusLabel_(new QLabel(currentContainer_)),
	  currentTitleLabel_(new QLabel(currentContainer_)),
	  currentLinkButton_(new QToolButton(currentContainer_)),

	  nextContainer_(new QWidget(scheduleGroup_)),
	  nextLayout_(new QVBoxLayout(nextContainer_)),
	  nextHeader_(new QHBoxLayout(nextContainer_)),
	  nextTitleRow_(new QHBoxLayout(nextContainer_)),
	  nextRoleLabel_(new QLabel(tr("NEXT"), nextContainer_)),
	  nextStatusLabel_(new QLabel(nextContainer_)),
	  nextTitleLabel_(new QLabel(nextContainer_)),
	  nextLinkButton_(new QToolButton(nextContainer_)),

	  // Log
	  logGroup_(new QGroupBox(tr("Operation Log"), this)),
	  logLayout_(new QVBoxLayout(logGroup_)),
	  consoleView_(new QTextEdit(logGroup_)),

	  // Bottom Controls
	  bottomControlLayout_(new QVBoxLayout(this)),
	  settingsButton_(new QPushButton(tr("Settings"), this)),
	  segmentNowButton_(new QPushButton(tr("Segment Now..."), this)),

	  // Cache Initialization
	  currentStatusText_(tr("IDLE")),
	  currentStatusColor_("#888888"),
	  currentNextTimeText_("--:--:--"),
	  currentTimeRemainingText_("--")
{
	setupUi();

	connect(startButton_, &QPushButton::clicked, this, &StreamSegmenterDock::startButtonClicked);
	connect(stopButton_, &QPushButton::clicked, this, &StreamSegmenterDock::stopButtonClicked);
	connect(segmentNowButton_, &QPushButton::clicked, this, &StreamSegmenterDock::segmentNowButtonClicked);
	connect(settingsButton_, &QPushButton::clicked, this, &StreamSegmenterDock::onSettingsButtonClicked);
}

void StreamSegmenterDock::setupUi()
{
	mainLayout_->setContentsMargins(4, 4, 4, 4);
	mainLayout_->setSpacing(6);

	// --- 1. Top Controls ---
	startButton_->setStyleSheet("font-weight: bold;");
	topControlLayout_->addWidget(startButton_, 1);
	topControlLayout_->addWidget(stopButton_, 1);
	mainLayout_->addLayout(topControlLayout_);

	// --- 2. Status Monitor ---
	statusLayout_->setContentsMargins(4, 8, 4, 8);
	const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	QFont monitorFont = fixedFont;
	monitorFont.setPointSize(12);
	monitorLabel_->setFont(monitorFont);
	monitorLabel_->setWordWrap(true);
	monitorLabel_->setAlignment(Qt::AlignCenter);
	statusLayout_->addWidget(monitorLabel_);
	mainLayout_->addWidget(statusGroup_);

	// --- 3. Schedule ---
	scheduleLayout_->setContentsMargins(4, 8, 4, 8);
	scheduleLayout_->setSpacing(12);

	// --- Current Block ---
	currentLayout_->setContentsMargins(4, 4, 4, 4);
	currentLayout_->setSpacing(2);

	currentHeader_->setContentsMargins(0, 0, 0, 0);
	currentRoleLabel_->setStyleSheet("color: #888888; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
	currentStatusLabel_->setStyleSheet("font-weight: bold; font-size: 11px;");
	currentHeader_->addWidget(currentRoleLabel_);
	currentHeader_->addStretch();
	currentHeader_->addWidget(currentStatusLabel_);
	currentLayout_->addLayout(currentHeader_);

	// Title Row
	currentTitleRow_->setContentsMargins(0, 0, 0, 0);
	currentTitleRow_->setSpacing(2);

	currentTitleLabel_->setWordWrap(true);
	currentTitleLabel_->setOpenExternalLinks(false);
	currentTitleLabel_->setMaximumHeight(45);
	currentTitleLabel_->setStyleSheet("font-size: 13px; padding-left: 2px; color: #e0e0e0;");

	currentLinkButton_->setText("↗");
	currentLinkButton_->setToolTip(tr("Open in Browser"));
	currentLinkButton_->setCursor(Qt::PointingHandCursor);
	currentLinkButton_->setStyleSheet("QToolButton {"
					  "    border: none;"
					  "    background: transparent;"
					  "    color: #666666;"
					  "    padding: 0px 2px;"
					  "    font-weight: bold;"
					  "}"
					  "QToolButton:hover {"
					  "    color: #4EC9B0;"
					  "}");
	currentLinkButton_->hide();

	currentTitleRow_->addWidget(currentTitleLabel_, 1);
	currentTitleRow_->addWidget(currentLinkButton_, 0, Qt::AlignTop);

	currentLayout_->addLayout(currentTitleRow_);

	currentContainer_->setStyleSheet("background-color: #2a2a2a; border-radius: 4px;");
	scheduleLayout_->addWidget(currentContainer_);

	// --- Next Block ---
	nextLayout_->setContentsMargins(4, 4, 4, 4);
	nextLayout_->setSpacing(2);

	nextHeader_->setContentsMargins(0, 0, 0, 0);
	nextRoleLabel_->setStyleSheet("color: #888888; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
	nextStatusLabel_->setStyleSheet("font-weight: bold; font-size: 11px;");
	nextHeader_->addWidget(nextRoleLabel_);
	nextHeader_->addStretch();
	nextHeader_->addWidget(nextStatusLabel_);
	nextLayout_->addLayout(nextHeader_);

	nextTitleRow_->setContentsMargins(0, 0, 0, 0);
	nextTitleRow_->setSpacing(2);

	nextTitleLabel_->setWordWrap(true);
	nextTitleLabel_->setOpenExternalLinks(false);
	nextTitleLabel_->setMaximumHeight(45);
	nextTitleLabel_->setStyleSheet("font-size: 13px; padding-left: 2px; color: #aaaaaa;");

	nextLinkButton_->setText("↗");
	nextLinkButton_->setToolTip(tr("Open in Browser"));
	nextLinkButton_->setCursor(Qt::PointingHandCursor);
	nextLinkButton_->setStyleSheet("QToolButton {"
				       "    border: none;"
				       "    background: transparent;"
				       "    color: #666666;"
				       "    padding: 0px 2px;"
				       "    font-weight: bold;"
				       "}"
				       "QToolButton:hover {"
				       "    color: #4EC9B0;"
				       "}");
	nextLinkButton_->hide();

	nextTitleRow_->addWidget(nextTitleLabel_, 1);
	nextTitleRow_->addWidget(nextLinkButton_, 0, Qt::AlignTop);

	nextLayout_->addLayout(nextTitleRow_);

	nextContainer_->setStyleSheet("background-color: #2a2a2a; border-radius: 4px;");
	scheduleLayout_->addWidget(nextContainer_);

	mainLayout_->addWidget(scheduleGroup_);

	// --- 4. Log ---
	logLayout_->setContentsMargins(0, 0, 0, 0);
	consoleView_->setReadOnly(true);
	consoleView_->setStyleSheet("QTextEdit {"
				    "   background-color: #1e1e1e;"
				    "   color: #e0e0e0;"
				    "   font-family: Consolas, 'Courier New', monospace;"
				    "   font-size: 11px;"
				    "   border: 1px solid #3c3c3c;"
				    "   border-top: none;"
				    "}");
	logGroup_->setStyleSheet(
		"QGroupBox { font-weight: bold; border: 1px solid #3c3c3c; margin-top: 6px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");
	logLayout_->addWidget(consoleView_);
	mainLayout_->addWidget(logGroup_, 1);

	// --- 5. Bottom Controls ---
	bottomControlLayout_->setSpacing(4);
	bottomControlLayout_->addWidget(settingsButton_);
	bottomControlLayout_->addWidget(segmentNowButton_);
	mainLayout_->addLayout(bottomControlLayout_);
}

void StreamSegmenterDock::logMessage([[maybe_unused]] int level, const QString &name,
				     const QMap<QString, QString> &context)
{
	if (name == "OBSStreamingStarted") {
		consoleView_->append(tr("OBS streaming started."));
	} else if (name == "YouTubeLiveStreamStatusChecking") {
		QString nextLiveStreamId = context.value("nextLiveStreamId");
		if (!nextLiveStreamId.isEmpty()) {
			consoleView_->append(
				tr("Checking YouTube live stream status (ID: %1)...").arg(nextLiveStreamId));
		} else {
			consoleView_->append(tr("Checking YouTube live stream status..."));
		}
	} else if (name == "YouTubeLiveStreamActive") {
		consoleView_->append(tr("YouTube live stream is active."));
	} else if (name == "YouTubeLiveStreamNotActiveYet") {
		QString remainingAttempts = context.value("remainingAttempts");
		if (!remainingAttempts.isEmpty()) {
			consoleView_->append(tr("YouTube live stream is not active yet. Remaining attempts: %1")
						     .arg(remainingAttempts));
		} else {
			consoleView_->append(tr("YouTube live stream is not active yet."));
		}
	} else if (name == "YouTubeLiveStreamStartTimeout") {
		consoleView_->append(tr("Timeout: YouTube live stream did not become active in time."));
	} else if (name == "YouTubeLiveBroadcastTransitioningToTesting") {
		consoleView_->append(tr("Transitioning YouTube live broadcast to 'testing' state..."));
	} else if (name == "YouTubeLiveBroadcastTransitionedToTesting") {
		consoleView_->append(tr("YouTube live broadcast transitioned to 'testing' state."));
	} else if (name == "YouTubeLiveBroadcastTransitioningToLive") {
		consoleView_->append(tr("Transitioning YouTube live broadcast to 'live' state..."));
	} else if (name == "YouTubeLiveBroadcastTransitionedToLive") {
		consoleView_->append(tr("YouTube live broadcast transitioned to 'live' state."));
	} else if (name == "UnsupportedIngestionTypeError") {
		QString msg;
		if (context.contains("type")) {
			msg = tr("Unsupported ingestion type: %1").arg(context["type"]);
		} else {
			msg = tr("Unsupported ingestion type: %1").arg(context["type"]);
		}
		consoleView_->append(msg);
	} else if (name == "YouTubeRTMPServiceCreated") {
		consoleView_->append(tr("YouTube RTMP service created."));
	} else if (name == "YouTubeHLSServiceCreated") {
		consoleView_->append(tr("YouTube HLS service created."));
	} else if (name == "StoppingCurrentStreamBeforeSegmenting") {
		consoleView_->append(tr("Stopping the current stream for segment switching. Please wait..."));
	} else if (name == "CompletingExistingLiveBroadcast") {
		QString msg;
		if (context.contains("title")) {
			msg = tr("Completing existing live broadcast: %1").arg(context["title"]);
		} else {
			msg = tr("Completing existing live broadcast: %1").arg(context["title"]);
		}
		consoleView_->append(msg);
	} else if (name == "YouTubeLiveBroadcastThumbnailSetting") {
		QString msg;
		if (context.contains("thumbnailFile")) {
			msg = tr("Setting YouTube live broadcast thumbnail: %1").arg(context["thumbnailFile"]);
		} else {
			msg = tr("Setting YouTube live broadcast thumbnail.");
		}
		consoleView_->append(msg);
	} else if (name == "YouTubeLiveBroadcastBinding") {
		consoleView_->append(tr("Binding YouTube live broadcast to stream..."));
	} else if (name == "YouTubeLiveBroadcastBound") {
		consoleView_->append(tr("YouTube live broadcast bound to stream."));
	} else if (name == "YouTubeLiveBroadcastThumbnailSet") {
		QString msg;
		if (context.contains("thumbnailFile")) {
			msg = tr("YouTube live broadcast thumbnail set: %1").arg(context["thumbnailFile"]);
		} else {
			msg = tr("YouTube live broadcast thumbnail set.");
		}
		consoleView_->append(msg);
	} else if (name == "YouTubeLiveBroadcastThumbnailMissing") {
		QString msg;
		if (context.contains("thumbnailFile")) {
			msg = tr("YouTube live broadcast thumbnail missing: %1").arg(context["thumbnailFile"]);
		} else {
			msg = tr("YouTube live broadcast thumbnail missing.");
		}
		consoleView_->append(msg);
	} else if (name == "YouTubeLiveBroadcastThumbnailSkippingDueToMissingVideoId") {
		consoleView_->append(tr("Skipping YouTube live broadcast thumbnail set due to missing video ID."));
	} else if (name == "YouTubeLiveBroadcastInserting") {
		consoleView_->append(tr("Creating new YouTube live broadcast..."));
	} else if (name == "YouTubeLiveBroadcastInserted") {
		QString msg;
		if (context.contains("title")) {
			msg = tr("YouTube live broadcast created: %1").arg(context["title"]);
		} else {
			msg = tr("YouTube live broadcast created.");
		}
		consoleView_->append(msg);
	}
}

void StreamSegmenterDock::onSettingsButtonClicked()
{
	auto logger = [this]() {
		std::scoped_lock lock(mutex_);
		return logger_;
	}();

	SettingsDialog settingsDialog(runtime_, authStore_, eventHandlerStore_, youTubeStore_, logger, this);
	settingsDialog.exec();
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
