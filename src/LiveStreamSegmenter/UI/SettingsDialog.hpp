/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * ... (ライセンス表記略) ...
 */

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QTabWidget>

#include <memory>
#include <future>
#include <optional>

#include <ILogger.hpp>
#include <AuthService.hpp>
#include <GoogleAuthManager.hpp>
#include <GoogleOAuth2Flow.hpp>
#include <YouTubeTypes.hpp>
#include "JsonDropArea.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(std::shared_ptr<const Logger::ILogger> logger, QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

private:
    void setupUi();

    const std::shared_ptr<const Logger::ILogger> logger_;

    // UI Components
    // Layouts & Containers
    QVBoxLayout *mainLayout_;
    
    QTabWidget *tabWidget_;
    QWidget *generalTab_;
    QVBoxLayout *generalTabLayout_;
    QWidget *youTubeTab_;
    QVBoxLayout *youTubeTabLayout_;

    QLabel *helpLabel_;

    QGroupBox *credGroup_;
    QVBoxLayout *credLayout_;
    // detailsLayout_ はローカル変数化したため削除

    QGroupBox *authGroup_;
    QVBoxLayout *authLayout_;

    QGroupBox *keyGroup_;
    QVBoxLayout *keyLayout_;
    QLabel *targetStreamKeyLabel_;
    // streamKeyRowLayout_ はローカル変数化したため削除

    // Existing Components
    JsonDropArea *dropArea_;
    QLineEdit *clientIdDisplay_;
    QLineEdit *clientSecretDisplay_;
    QPushButton *saveButton_;
    QPushButton *authButton_;
    QLabel *statusLabel_;

    QComboBox *streamKeyCombo_;
    QPushButton *refreshKeysBtn_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
