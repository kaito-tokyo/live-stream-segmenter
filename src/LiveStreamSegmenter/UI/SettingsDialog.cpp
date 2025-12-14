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
	  infoLabel_(new QLabel(
		  tr("<b>Step 1:</b> Import Credentials.<br><b>Step 2:</b> Authenticate.<br><b>Step 3:</b> Select Stream Key."),
		  this)),
	  credGroup_(new QGroupBox(tr("Credentials"), this)),
	  credLayout_(new QVBoxLayout(credGroup_)),
	  detailsLayout_(new QFormLayout()),
	  authGroup_(new QGroupBox(tr("Authorization"), this)),
	  authLayout_(new QVBoxLayout(authGroup_)),
	  keyGroup_(new QGroupBox(tr("Stream Settings"), this)),
	  keyLayout_(new QVBoxLayout(keyGroup_)),
	  keyHeaderLayout_(new QHBoxLayout()),
	  targetStreamKeyLabel_(new QLabel(tr("Target Stream Key:"), this)),
	  // Existing Components initialization
	  dropArea_(new JsonDropArea(logger_, this)),
	  clientIdDisplay_(new QLineEdit(this)),
	  clientSecretDisplay_(new QLineEdit(this)),
	  loadJsonButton_(new QPushButton(tr("Select File..."), this)),
	  saveButton_(new QPushButton(tr("2. Save Credentials"), this)),
	  authButton_(new QPushButton(tr("3. Authenticate"), this)),
	  statusLabel_(new QLabel(this)),
	  streamKeyCombo_(new QComboBox(this)),
	  refreshKeysBtn_(new QPushButton(tr("Reload Keys"), this))
{
	setWindowTitle(tr("Settings"));
	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint |
		       Qt::WindowTitleHint);

	setupUi();
}

void SettingsDialog::setupUi()
{
	// Main Layout Config
	mainLayout_->setSpacing(16);
	mainLayout_->setContentsMargins(24, 24, 24, 24);

	// Info Label Config
	infoLabel_->setStyleSheet("color: #aaaaaa; font-size: 11px;");
	mainLayout_->addWidget(infoLabel_);

	// Drop Area Config
	dropArea_->setText(tr("Drag & Drop credentials.json here"));
	dropArea_->setAlignment(Qt::AlignCenter);
	dropArea_->setStyleSheet("border: 2px dashed #666; color: #ccc;");

	// Credentials Group Config
	credLayout_->addWidget(dropArea_);

	// Details Form Layout Config
	clientIdDisplay_->setReadOnly(true);
	clientIdDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout_->addRow(tr("Client ID:"), clientIdDisplay_);

	clientSecretDisplay_->setReadOnly(true);
	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout_->addRow(tr("Client Secret:"), clientSecretDisplay_);

	credLayout_->addLayout(detailsLayout_);

	saveButton_->setEnabled(false);
	credLayout_->addWidget(saveButton_);
	mainLayout_->addWidget(credGroup_);

	// Auth Group Config
	authButton_->setEnabled(false);
	statusLabel_->setAlignment(Qt::AlignCenter);
	authLayout_->addWidget(authButton_);
	authLayout_->addWidget(statusLabel_);
	mainLayout_->addWidget(authGroup_);

	// Stream Key Group Config
	keyHeaderLayout_->addWidget(targetStreamKeyLabel_);
	keyHeaderLayout_->addStretch();

	refreshKeysBtn_->setCursor(Qt::PointingHandCursor);
	refreshKeysBtn_->setFixedWidth(100);
	refreshKeysBtn_->setEnabled(false);
	keyHeaderLayout_->addWidget(refreshKeysBtn_);

	keyLayout_->addLayout(keyHeaderLayout_);

	streamKeyCombo_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyCombo_->setEnabled(false);
	keyLayout_->addWidget(streamKeyCombo_);

	mainLayout_->addWidget(keyGroup_);
	mainLayout_->addStretch();

	resize(420, 600);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
