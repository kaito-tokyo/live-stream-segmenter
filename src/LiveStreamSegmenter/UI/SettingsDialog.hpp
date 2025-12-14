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
#include <QMimeData>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <memory>
#include <future>
#include <optional>

#include <ILogger.hpp>

#include <AuthService.hpp>
#include <GoogleAuthManager.hpp>
#include <GoogleOAuth2Flow.hpp>
#include <YouTubeTypes.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

class JsonDropArea : public QLabel {
	Q_OBJECT
public:
	explicit JsonDropArea(QWidget *parent = nullptr);

signals:
	void fileDropped(const QString &filePath);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
};

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	SettingsDialog(std::shared_ptr<const Logger::ILogger> logger, QWidget *parent = nullptr);
	~SettingsDialog() override = default;

private:
	void setupUi();

	const std::shared_ptr<const Logger::ILogger> logger_;

	// UI Components
	// Layouts & Containers (Newly added members)
	QVBoxLayout *mainLayout_;
	QLabel *infoLabel_;

	QGroupBox *credGroup_;
	QVBoxLayout *credLayout_;
	QFormLayout *detailsLayout_;

	QGroupBox *authGroup_;
	QVBoxLayout *authLayout_;

	QGroupBox *keyGroup_;
	QVBoxLayout *keyLayout_;
	QHBoxLayout *keyHeaderLayout_;
	QLabel *targetStreamKeyLabel_;

	// Existing Components
	JsonDropArea *dropArea_;
	QLineEdit *clientIdDisplay_;
	QLineEdit *clientSecretDisplay_;
	QPushButton *loadJsonButton_;
	QPushButton *saveButton_;
	QPushButton *authButton_;
	QLabel *statusLabel_;

	QComboBox *streamKeyCombo_;
	QPushButton *refreshKeysBtn_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
