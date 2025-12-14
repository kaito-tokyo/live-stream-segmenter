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

#include <QFormLayout>
#include <QMimeData>
#include <QUrl>

using namespace KaitoTokyo::Logger;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

SettingsDialog::SettingsDialog(std::shared_ptr<const ILogger> logger, QWidget *parent)
	: QDialog(parent),
	  logger_(std::move(logger)),

	  // 1. Main Structure
	  mainLayout_(new QVBoxLayout(this)),
	  tabWidget_(new QTabWidget(this)),

	  // 2. General Tab
	  generalTab_(new QWidget(this)),
	  generalTabLayout_(new QVBoxLayout(generalTab_)),

	  // 3. YouTube Tab
	  youTubeTab_(new QWidget(this)),
	  youTubeTabLayout_(new QVBoxLayout(youTubeTab_)),
	  helpLabel_(new QLabel(this)),

	  // 4. Credentials Group
	  credGroup_(new QGroupBox(this)),
	  credLayout_(new QVBoxLayout(credGroup_)),
	  dropArea_(new JsonDropArea(this)),
	  clientIdDisplay_(new QLineEdit(this)),
	  clientSecretDisplay_(new QLineEdit(this)),

	  // 5. Authorization Group
	  authGroup_(new QGroupBox(this)),
	  authLayout_(new QVBoxLayout(authGroup_)),
	  authButton_(new QPushButton(this)),
	  statusLabel_(new QLabel(this)),

	  // 6. Stream Settings Group
	  keyGroup_(new QGroupBox(this)),
	  keyLayout_(new QVBoxLayout(keyGroup_)),
	  streamKeyLabelA_(new QLabel(this)),
	  streamKeyComboA_(new QComboBox(this)),
	  streamKeyLabelB_(new QLabel(this)),
	  streamKeyComboB_(new QComboBox(this)),

	  // 7. Dialog Buttons
	  buttonBox_(
		  new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this))
{
	setupUi();
}

void SettingsDialog::setupUi()
{
	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint |
		       Qt::WindowTitleHint);

	setWindowTitle(tr("Settings"));

	// Main Layout Config
	mainLayout_->setSpacing(0);
	mainLayout_->setContentsMargins(12, 12, 12, 12);

	// --- General Tab Config ---
	generalTabLayout_->setSpacing(16);
	generalTabLayout_->setContentsMargins(16, 16, 16, 16);
	generalTabLayout_->addStretch();

	// --- YouTube Tab Config ---
	youTubeTabLayout_->setSpacing(16);
	youTubeTabLayout_->setContentsMargins(16, 16, 16, 16);

	// Help Label
	helpLabel_->setText(tr("<a href=\"https://kaito-tokyo.github.io/live-stream-segmenter/setup\">"
			       "View Official Setup Instructions for YouTube"
			       "</a>"));
	helpLabel_->setOpenExternalLinks(true);
	helpLabel_->setWordWrap(true);
	youTubeTabLayout_->addWidget(helpLabel_);

	// --- 1. OAuth2 Client Credentials Group ---
	credGroup_->setTitle(tr("1. OAuth2 Client Credentials"));

	// Drop Area
	dropArea_->setText(tr("Drag & Drop credentials.json here"));
	dropArea_->setAlignment(Qt::AlignCenter);
	dropArea_->setStyleSheet(R"(
        QLabel {
            border: 2px dashed palette(highlight);
            color: palette(text);
            border-radius: 6px;
            padding: 16px;
        }
    )");

	credLayout_->addWidget(dropArea_);
	credLayout_->addSpacing(16);

	// Details Form
	QFormLayout *detailsLayout = new QFormLayout();
	detailsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	clientIdDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Client ID"), clientIdDisplay_);

	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Client Secret"), clientSecretDisplay_);

	credLayout_->addLayout(detailsLayout);
	youTubeTabLayout_->addWidget(credGroup_);

	// --- 2. OAuth2 Authorization Group ---
	authGroup_->setTitle(tr("2. OAuth2 Authorization"));

	authButton_->setText(tr("Request Authorization"));
	authButton_->setEnabled(false);

	statusLabel_->setAlignment(Qt::AlignCenter);
	statusLabel_->setText(tr("Unauthorized"));

	authLayout_->addWidget(authButton_);
	authLayout_->addWidget(statusLabel_);

	youTubeTabLayout_->addWidget(authGroup_);

	// --- 3. Stream Settings Group ---
	keyGroup_->setTitle(tr("3. Stream Settings"));

	QFormLayout *streamKeyFormLayout = new QFormLayout();
	streamKeyFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	// Key A
	streamKeyLabelA_->setText(tr("Stream Key A"));
	streamKeyComboA_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyComboA_->setEnabled(false);
	streamKeyComboA_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	streamKeyFormLayout->addRow(streamKeyLabelA_, streamKeyComboA_);

	// Key B
	streamKeyLabelB_->setText(tr("Stream Key B"));
	streamKeyComboB_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyComboB_->setEnabled(false);
	streamKeyComboB_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	streamKeyFormLayout->addRow(streamKeyLabelB_, streamKeyComboB_);

	keyLayout_->addLayout(streamKeyFormLayout);
	youTubeTabLayout_->addWidget(keyGroup_);

	youTubeTabLayout_->addStretch();

	// --- Finalize Tabs ---
	tabWidget_->addTab(generalTab_, tr("General"));
	tabWidget_->addTab(youTubeTab_, tr("YouTube"));
	mainLayout_->addWidget(tabWidget_);

	tabWidget_->setCurrentWidget(youTubeTab_);

	// --- Dialog Buttons ---
	connect(buttonBox_, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);

	mainLayout_->addWidget(buttonBox_);

	// Window Sizing
	mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
	resize(500, 700);
}

void SettingsDialog::accept()
{
	QDialog::accept();
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
