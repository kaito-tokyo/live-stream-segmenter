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

#include "SettingsDialog.hpp"

#include <thread>
#include <future>

#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <obs-module.h>
#include <QTimer>

#include <YouTubeApiClient.hpp>
#include <GoogleTokenState.hpp>
#include <QMimeData>

using namespace KaitoTokyo::Logger;

using namespace KaitoTokyo::LiveStreamSegmenter::API;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

SettingsDialog::SettingsDialog(std::shared_ptr<const ILogger> logger, QWidget *parent)
	: QDialog(parent),
	  logger_(std::move(logger)),
	  // Layouts & Containers initialization
	  mainLayout_(new QVBoxLayout(this)),
	  tabWidget_(new QTabWidget(this)),
	  generalTab_(new QWidget(this)),
	  generalTabLayout_(new QVBoxLayout(generalTab_)),
	  youTubeTab_(new QWidget(this)),
	  youTubeTabLayout_(new QVBoxLayout(youTubeTab_)),
	  helpLabel_(new QLabel(this)),
	  credGroup_(new QGroupBox(this)),
	  credLayout_(new QVBoxLayout(credGroup_)),
	  detailsLayout_(new QFormLayout()),
	  authGroup_(new QGroupBox(this)),
	  authLayout_(new QVBoxLayout(authGroup_)),
	  keyGroup_(new QGroupBox(this)),
	  keyLayout_(new QVBoxLayout(keyGroup_)),
	  keyHeaderLayout_(new QHBoxLayout()),
	  targetStreamKeyLabel_(new QLabel(this)),
	  // Existing Components initialization
	  dropArea_(new JsonDropArea(this)),
	  clientIdDisplay_(new QLineEdit(this)),
	  clientSecretDisplay_(new QLineEdit(this)),
	  saveButton_(new QPushButton(this)),
	  authButton_(new QPushButton(this)),
	  statusLabel_(new QLabel(this)),
	  streamKeyCombo_(new QComboBox(this)),
	  refreshKeysBtn_(new QPushButton(this))
{
	setupUi();
}

void SettingsDialog::setupUi()
{
	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint |
		       Qt::WindowTitleHint);

	// Window Title
	setWindowTitle(tr("Settings"));

	// Main Layout Config
	mainLayout_->setSpacing(0);
	mainLayout_->setContentsMargins(12, 12, 12, 12);

	generalTabLayout_->setSpacing(16);
	generalTabLayout_->setContentsMargins(16, 16, 16, 16);
	generalTabLayout_->addStretch(); // 上詰めにするためのストレッチ

	youTubeTabLayout_->setSpacing(16);
	youTubeTabLayout_->setContentsMargins(16, 16, 16, 16);

	// Help Label (Link)
	helpLabel_->setText(tr("<a href=\"https://kaito-tokyo.github.io/live-stream-segmenter/setup\">"
			       "How to set up Live Stream Segmenter with YouTube?"
			       "</a>"));
	helpLabel_->setOpenExternalLinks(true);
	helpLabel_->setWordWrap(true);
	youTubeTabLayout_->addWidget(helpLabel_);

	// --- Credentials Group ---
	credGroup_->setTitle(tr("Credentials")); // ★ここで設定

	// Drop Area Config
	dropArea_->setText(tr("Drag & Drop credentials.json here"));
	dropArea_->setAlignment(Qt::AlignCenter);
	dropArea_->setStyleSheet("border: 2px dashed #666; color: #ccc;");

	credLayout_->addWidget(dropArea_);

	// Details Form Layout Config
	clientIdDisplay_->setReadOnly(true);
	clientIdDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout_->addRow(tr("Client ID:"), clientIdDisplay_); // ここは元々 setupUi 内

	clientSecretDisplay_->setReadOnly(true);
	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout_->addRow(tr("Client Secret:"), clientSecretDisplay_); // ここも元々 setupUi 内

	credLayout_->addLayout(detailsLayout_);

	saveButton_->setText(tr("2. Save Credentials")); // ★ここで設定
	saveButton_->setEnabled(false);
	credLayout_->addWidget(saveButton_);

	youTubeTabLayout_->addWidget(credGroup_);

	// --- Authorization Group ---
	authGroup_->setTitle(tr("Authorization")); // ★ここで設定

	authButton_->setText(tr("3. Authenticate")); // ★ここで設定
	authButton_->setEnabled(false);

	statusLabel_->setAlignment(Qt::AlignCenter);

	authLayout_->addWidget(authButton_);
	authLayout_->addWidget(statusLabel_);

	youTubeTabLayout_->addWidget(authGroup_);

	// --- Stream Key Group ---
	keyGroup_->setTitle(tr("Stream Settings")); // ★ここで設定

	targetStreamKeyLabel_->setText(tr("Target Stream Key:")); // ★ここで設定
	keyHeaderLayout_->addWidget(targetStreamKeyLabel_);
	keyHeaderLayout_->addStretch();

	refreshKeysBtn_->setText(tr("Reload Keys")); // ★ここで設定
	refreshKeysBtn_->setCursor(Qt::PointingHandCursor);
	refreshKeysBtn_->setFixedWidth(100);
	refreshKeysBtn_->setEnabled(false);
	keyHeaderLayout_->addWidget(refreshKeysBtn_);

	keyLayout_->addLayout(keyHeaderLayout_);

	streamKeyCombo_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyCombo_->setEnabled(false);
	keyLayout_->addWidget(streamKeyCombo_);

	youTubeTabLayout_->addWidget(keyGroup_);
	youTubeTabLayout_->addStretch();

	tabWidget_->addTab(generalTab_, tr("General"));
	tabWidget_->addTab(youTubeTab_, tr("YouTube"));
	mainLayout_->addWidget(tabWidget_);

	tabWidget_->setCurrentWidget(youTubeTab_);

	mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
	resize(500, 650);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
