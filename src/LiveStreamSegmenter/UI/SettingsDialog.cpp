/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * ... (ライセンス表記略) ...
 */

#include "SettingsDialog.hpp"

// ... (インクルードはそのまま) ...
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
      
      // Tabs
      generalTab_(new QWidget(this)),
      generalTabLayout_(new QVBoxLayout(generalTab_)),
      youTubeTab_(new QWidget(this)),
      youTubeTabLayout_(new QVBoxLayout(youTubeTab_)),
      
      helpLabel_(new QLabel(this)),
      
      // Groups
      credGroup_(new QGroupBox(this)),
      credLayout_(new QVBoxLayout(credGroup_)),
      // detailsLayout_ はローカル変数にするため削除

      authGroup_(new QGroupBox(this)),
      authLayout_(new QVBoxLayout(authGroup_)),
      
      keyGroup_(new QGroupBox(this)),
      keyLayout_(new QVBoxLayout(keyGroup_)),
      targetStreamKeyLabel_(new QLabel(this)),
      // streamKeyRowLayout_ はローカル変数にするため削除

      // Components
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

    // --- General Tab Config ---
    generalTabLayout_->setSpacing(16);
    generalTabLayout_->setContentsMargins(16, 16, 16, 16);
    generalTabLayout_->addStretch();

    // --- YouTube Tab Config ---
    youTubeTabLayout_->setSpacing(16);
    youTubeTabLayout_->setContentsMargins(16, 16, 16, 16);

    // Help Label (Link)
    helpLabel_->setText(tr("<a href=\"https://kaito-tokyo.github.io/live-stream-segmenter/setup\">"
                           "View Official Setup Instructions for YouTube"
                           "</a>"));
    helpLabel_->setOpenExternalLinks(true);
    helpLabel_->setWordWrap(true);
    youTubeTabLayout_->addWidget(helpLabel_);

    // --- 1. OAuth2 Client Credentials Group ---
    // ★変更: タイトルに番号を追加
    credGroup_->setTitle(tr("1. OAuth2 Client Credentials"));

    // Drop Area Config
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

    // Details Form Layout Config
    // ★変更: ローカル変数として宣言・生成 (親は指定しない)
    QFormLayout *detailsLayout = new QFormLayout();
    
    detailsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    clientIdDisplay_->setReadOnly(true);
    clientIdDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    clientIdDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
    detailsLayout->addRow(tr("Client ID"), clientIdDisplay_);

    clientSecretDisplay_->setReadOnly(true);
    clientSecretDisplay_->setEchoMode(QLineEdit::Password);
    clientSecretDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    clientSecretDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
    detailsLayout->addRow(tr("Client Secret"), clientSecretDisplay_);

    // ★ここで credLayout_ に追加することで所有権が移動する
    credLayout_->addLayout(detailsLayout);

    saveButton_->setText(tr("Save Credentials"));
    saveButton_->setEnabled(false);
    credLayout_->addWidget(saveButton_);

    youTubeTabLayout_->addWidget(credGroup_);

    // --- 2. OAuth2 Authorization Group ---
    // ★変更: タイトルに番号を追加
    authGroup_->setTitle(tr("2. OAuth2 Authorization"));

    authButton_->setText(tr("Request Authorization"));
    authButton_->setEnabled(false);

    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setText(tr("Unauthorized"));

    authLayout_->addWidget(authButton_);
    authLayout_->addWidget(statusLabel_);

    youTubeTabLayout_->addWidget(authGroup_);

    // --- 3. Stream Settings Group ---
    // ★変更: タイトルに番号を追加
    keyGroup_->setTitle(tr("3. Stream Settings"));

    targetStreamKeyLabel_->setText(tr("Target Stream Key:"));
    keyLayout_->addWidget(targetStreamKeyLabel_);

    // コンボボックスとリロードボタンを横並びにするレイアウト
    // ★変更: ローカル変数として宣言・生成 (親は指定しない)
    QHBoxLayout *streamKeyRowLayout = new QHBoxLayout();
    
    streamKeyRowLayout->setSpacing(8);

    streamKeyCombo_->setPlaceholderText(tr("Authenticate to fetch keys..."));
    streamKeyCombo_->setEnabled(false);
    streamKeyCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    refreshKeysBtn_->setText(tr("Reload"));
    refreshKeysBtn_->setCursor(Qt::PointingHandCursor);
    refreshKeysBtn_->setEnabled(false);

    streamKeyRowLayout->addWidget(streamKeyCombo_);
    streamKeyRowLayout->addWidget(refreshKeysBtn_);

    // ★ここで keyLayout_ に追加することで所有権が移動する
    keyLayout_->addLayout(streamKeyRowLayout);

    youTubeTabLayout_->addWidget(keyGroup_);
    youTubeTabLayout_->addStretch();

    // --- Finalize Tabs ---
    tabWidget_->addTab(generalTab_, tr("General"));
    tabWidget_->addTab(youTubeTab_, tr("YouTube"));
    mainLayout_->addWidget(tabWidget_);

    // Default to YouTube tab
    tabWidget_->setCurrentWidget(youTubeTab_);

    // Window Sizing
    mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
    
    resize(500, 650);
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
