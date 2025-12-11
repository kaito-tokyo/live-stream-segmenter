/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "StreamSegmenterDock.hpp"
#include <QFontDatabase>
#include <QScrollBar>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <fmt/format.h>

#include <NullLogger.hpp>

#include "SettingsDialog.hpp"

using namespace KaitoTokyo::Logger;

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

namespace {

}

StreamSegmenterDock::StreamSegmenterDock(std::shared_ptr<Logger::ILogger> logger, QWidget *parent)
	: QDockWidget(parent),
	  logger_(std::move(logger)),
	  // --- Member Initialization List ---
	  mainWidget_(new QWidget(this)),
	  titleBarWidget_(new QWidget(this)),
	  mainLayout_(new QVBoxLayout(mainWidget_)),

	  // Top Controls
	  topControlLayout_(new QHBoxLayout()),
	  startButton_(new QPushButton(tr("Start"), mainWidget_)),
	  stopButton_(new QPushButton(tr("Stop"), mainWidget_)),

	  // Status Monitor
	  statusGroup_(new QGroupBox(tr("System Monitor"), mainWidget_)),
	  statusLayout_(new QVBoxLayout(statusGroup_)),
	  monitorLabel_(new QLabel(statusGroup_)),

	  // Schedule
	  scheduleGroup_(new QGroupBox(tr("Broadcast Schedule"), mainWidget_)),
	  scheduleLayout_(new QVBoxLayout(scheduleGroup_)),

	  currentContainer_(new QWidget(scheduleGroup_)),
	  currentLayout_(new QVBoxLayout(currentContainer_)),
	  currentRoleLabel_(new QLabel("CURRENT", currentContainer_)),
	  currentStatusLabel_(new QLabel(currentContainer_)),
	  currentTitleLabel_(new QLabel(currentContainer_)),
	  currentLinkButton_(new QToolButton(currentContainer_)), // 【修正】

	  nextContainer_(new QWidget(scheduleGroup_)),
	  nextLayout_(new QVBoxLayout(nextContainer_)),
	  nextRoleLabel_(new QLabel("NEXT", nextContainer_)),
	  nextStatusLabel_(new QLabel(nextContainer_)),
	  nextTitleLabel_(new QLabel(nextContainer_)),
	  nextLinkButton_(new QToolButton(nextContainer_)), // 【修正】

	  // Log
	  logGroup_(new QGroupBox(tr("Operation Log"), mainWidget_)),
	  logLayout_(new QVBoxLayout(logGroup_)),
	  consoleView_(new QTextEdit(logGroup_)),

	  // Bottom Controls
	  bottomControlLayout_(new QVBoxLayout()),
	  settingsButton_(new QPushButton(tr("Settings"), mainWidget_)),
	  segmentNowButton_(new QPushButton(tr("Segment Now..."), mainWidget_)), // 【修正】

	  // Cache Initialization
	  currentStatusText_("IDLE"),
	  currentStatusColor_("#888888"),
	  currentNextTimeText_("--:--:--"),
	  currentTimeRemainingText_("--")
{
	setObjectName("StreamSegmenterDock");
	setWindowTitle(tr("Stream Segmenter"));
	setTitleBarWidget(titleBarWidget_);

	setupUi();

	// リンクボタンの接続
	connect(currentLinkButton_, &QToolButton::clicked, this, &StreamSegmenterDock::onLinkButtonClicked);
	connect(nextLinkButton_, &QToolButton::clicked, this, &StreamSegmenterDock::onLinkButtonClicked);

	connect(this, &StreamSegmenterDock::logRequest, this, &StreamSegmenterDock::onLogRequest, Qt::QueuedConnection);

	connect(startButton_, &QPushButton::clicked, this, &StreamSegmenterDock::onStartClicked);
	connect(stopButton_, &QPushButton::clicked, this, &StreamSegmenterDock::onStopClicked);
	connect(settingsButton_, &QPushButton::clicked, this, &StreamSegmenterDock::onSettingsClicked);
	connect(segmentNowButton_, &QPushButton::clicked, this, &StreamSegmenterDock::onSegmentNowClicked);

	setWidget(mainWidget_);

	updateMonitorLabel();
	startButton_->setEnabled(true);
	stopButton_->setEnabled(false);

	info("Live Stream Segmenter initialized.");

	updateCurrentStream("(No Active Stream)", "---");
	updateNextStream("(Pending)", "Waiting");
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

	auto *currentHeader = new QHBoxLayout();
	currentHeader->setContentsMargins(0, 0, 0, 0);
	currentRoleLabel_->setStyleSheet("color: #888888; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
	currentStatusLabel_->setStyleSheet("font-weight: bold; font-size: 11px;");
	currentHeader->addWidget(currentRoleLabel_);
	currentHeader->addStretch();
	currentHeader->addWidget(currentStatusLabel_);
	currentLayout_->addLayout(currentHeader);

	// Title Row
	auto *currentTitleRow = new QHBoxLayout();
	currentTitleRow->setContentsMargins(0, 0, 0, 0);
	currentTitleRow->setSpacing(2);

	currentTitleLabel_->setWordWrap(true);
	currentTitleLabel_->setOpenExternalLinks(false);
	currentTitleLabel_->setMaximumHeight(45);
	currentTitleLabel_->setStyleSheet("font-size: 13px; padding-left: 2px; color: #e0e0e0;");

	// リンクボタン設定
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

	currentTitleRow->addWidget(currentTitleLabel_, 1);
	currentTitleRow->addWidget(currentLinkButton_, 0, Qt::AlignTop);

	currentLayout_->addLayout(currentTitleRow);

	currentContainer_->setStyleSheet("background-color: #2a2a2a; border-radius: 4px;");
	scheduleLayout_->addWidget(currentContainer_);

	// --- Next Block ---
	nextLayout_->setContentsMargins(4, 4, 4, 4);
	nextLayout_->setSpacing(2);

	auto *nextHeader = new QHBoxLayout();
	nextHeader->setContentsMargins(0, 0, 0, 0);
	nextRoleLabel_->setStyleSheet("color: #888888; font-size: 10px; font-weight: bold; letter-spacing: 1px;");
	nextStatusLabel_->setStyleSheet("font-weight: bold; font-size: 11px;");
	nextHeader->addWidget(nextRoleLabel_);
	nextHeader->addStretch();
	nextHeader->addWidget(nextStatusLabel_);
	nextLayout_->addLayout(nextHeader);

	auto *nextTitleRow = new QHBoxLayout();
	nextTitleRow->setContentsMargins(0, 0, 0, 0);
	nextTitleRow->setSpacing(2);

	nextTitleLabel_->setWordWrap(true);
	nextTitleLabel_->setOpenExternalLinks(false);
	nextTitleLabel_->setMaximumHeight(45);
	nextTitleLabel_->setStyleSheet("font-size: 13px; padding-left: 2px; color: #aaaaaa;");

	// リンクボタン設定
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

	nextTitleRow->addWidget(nextTitleLabel_, 1);
	nextTitleRow->addWidget(nextLinkButton_, 0, Qt::AlignTop);

	nextLayout_->addLayout(nextTitleRow);

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

void StreamSegmenterDock::onLinkButtonClicked()
{
	auto *button = qobject_cast<QToolButton *>(sender()); // 【修正】
	if (button) {
		QString urlStr = button->property("url").toString();
		if (!urlStr.isEmpty()) {
			QDesktopServices::openUrl(QUrl(urlStr));
		}
	}
}

void StreamSegmenterDock::updateCurrentStream(const QString &title, const QString &status, const QString &url)
{
	currentTitleLabel_->setText(title);

	QString tooltip = title;
	if (!url.isEmpty()) {
		tooltip += "\n" + url;
		currentLinkButton_->setProperty("url", url);
		currentLinkButton_->show();
	} else {
		currentLinkButton_->setProperty("url", "");
		currentLinkButton_->hide();
	}
	currentTitleLabel_->setToolTip(tooltip);

	if (status == "LIVE") {
		currentStatusLabel_->setText(QString("<span style='color: #4CAF50;'>● %1</span>").arg(status));
	} else {
		currentStatusLabel_->setText(status);
	}
}

void StreamSegmenterDock::updateNextStream(const QString &title, const QString &status, const QString &url)
{
	nextTitleLabel_->setText(title);

	QString tooltip = title;
	if (!url.isEmpty()) {
		tooltip += "\n" + url;
		nextLinkButton_->setProperty("url", url);
		nextLinkButton_->show();
	} else {
		nextLinkButton_->setProperty("url", "");
		nextLinkButton_->hide();
	}
	nextTitleLabel_->setToolTip(tooltip);

	nextStatusLabel_->setText(status);
}

// ... (他は変更なし) ...

void StreamSegmenterDock::onSettingsClicked()
{
	// auto authManager = std::make_shared<KaitoTokyo::LiveStreamSegmenter::Auth::GoogleAuthManager>("", "", logger_);
	// SettingsDialog dialog(authManager, logger, this);
	// dialog.exec(); // モーダルダイアログとして表示（閉じるまで親を操作不可）
}

void StreamSegmenterDock::updateMonitorLabel()
{
	QString html =
		QString("<span style='color: %1; font-weight: bold;'>%2</span>"
			"<span style='color: #666666;'> / </span>"
			"Next <span style='font-weight: bold;'>%3</span>"
			"<span style='color: #666666;'> / </span>"
			"Rem. <span style='font-weight: bold;'>%4</span>")
			.arg(currentStatusColor_, currentStatusText_, currentNextTimeText_, currentTimeRemainingText_);
	monitorLabel_->setText(html);
}

void StreamSegmenterDock::onStartClicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("Confirm Start"),
				      tr("Are you sure you want to start the automatic segmentation service?"),
				      QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		startButton_->setEnabled(false);
		stopButton_->setEnabled(true);
		setSystemStatus("RUNNING", "#4CAF50");

		QString longTitle = "Endurance Stream Part 1 - Very long title test.";
		updateCurrentStream(longTitle, "LIVE", "https://youtube.com");
		updateNextStream("Endurance Stream Part 2", "READY", "https://youtube.com");

		info("Service started.");
	} else {
		info("Start cancelled.");
	}
}

void StreamSegmenterDock::onStopClicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(
		this, tr("Confirm Stop"),
		tr("Are you sure you want to stop the service?\nAutomatic segmentation will be disabled."),
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		startButton_->setEnabled(true);
		stopButton_->setEnabled(false);
		setSystemStatus("STOPPED", "#F44747");

		updateCurrentStream("(No Active Stream)", "---");
		updateNextStream("(Pending)", "Waiting");
		warn("Service stopped.");
	} else {
		info("Stop cancelled.");
	}
}

void StreamSegmenterDock::onSegmentNowClicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("Confirm Segmentation"),
				      tr("Are you sure you want to stop the current stream and start the next one?"),
				      QMessageBox::Yes | QMessageBox::No);
	if (reply == QMessageBox::Yes) {
		info("Manual segmentation requested.");
	}
}

void StreamSegmenterDock::log(LogLevel level, std::string_view message) const noexcept
{
	emit const_cast<StreamSegmenterDock *>(this)->logRequest(
		static_cast<int>(level), QString::fromUtf8(message.data(), static_cast<int>(message.size())));
}

void StreamSegmenterDock::onLogRequest(int levelInt, const QString &message)
{
	LogLevel level = static_cast<LogLevel>(levelInt);
	QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss]");
	QString color, levelStr;

	switch (level) {
	case LogLevel::Debug:
		color = "#808080";
		levelStr = "DBG";
		break;
	case LogLevel::Info:
		color = "#569CD6";
		levelStr = "INFO";
		break;
	case LogLevel::Warn:
		color = "#DCDCAA";
		levelStr = "WARN";
		break;
	case LogLevel::Error:
		color = "#F44747";
		levelStr = "ERR";
		break;
	}

	QString html = QString("<span style='color:#666666;'>%1</span> "
			       "<span style='color:%2; font-weight:bold;'>[%3]</span> %4")
			       .arg(timeStr, color, levelStr, message);

	consoleView_->append(html);
	consoleView_->verticalScrollBar()->setValue(consoleView_->verticalScrollBar()->maximum());
}

void StreamSegmenterDock::setSystemStatus(const QString &statusText, const QString &colorCode)
{
	currentStatusText_ = statusText;
	currentStatusColor_ = colorCode;
	updateMonitorLabel();
}

void StreamSegmenterDock::setNextSegmentationTime(const QDateTime &time)
{
	currentNextTimeText_ = time.toString("hh:mm ap");
	updateMonitorLabel();
}

void StreamSegmenterDock::setTimeRemaining(int remainingSeconds)
{
	QString newText;
	if (remainingSeconds < 0) {
		newText = "--";
	} else if (remainingSeconds < 60) {
		newText = "< 1m";
	} else {
		const int hours = remainingSeconds / 3600;
		const int minutes = (remainingSeconds % 3600) / 60;

		std::string s;
		if (hours > 0) {
			s = fmt::format("{}h {:02}m", hours, minutes);
		} else {
			s = fmt::format("{}m", minutes);
		}
		newText = QString::fromStdString(s);
	}

	if (currentTimeRemainingText_ != newText) {
		currentTimeRemainingText_ = newText;
		updateMonitorLabel();
	}
}

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
