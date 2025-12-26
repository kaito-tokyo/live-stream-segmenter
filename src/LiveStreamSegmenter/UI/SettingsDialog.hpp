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

#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <ILogger.hpp>

#include <AuthStore.hpp>
#include <GoogleOAuth2Flow.hpp>

#include "GoogleOAuth2FlowCallbackServer.hpp"
#include "JsonDropArea.hpp"

#ifdef __APPLE__
#include <jthread.hpp>
#else
#include <thread>
#endif

namespace KaitoTokyo::LiveStreamSegmenter::UI {

#ifdef __APPLE__
namespace jthread_ns = josuttis;
#else
namespace jthread_ns = std;
#endif

struct SettingsDialogGoogleOAuth2ClientCredentials {
	QString client_id;
	QString client_secret;
};

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	SettingsDialog(std::shared_ptr<Store::AuthStore> authStore, std::shared_ptr<const Logger::ILogger> logger,
		       QWidget *parent = nullptr);
	~SettingsDialog() override;

public slots:
	void accept() override;

private slots:
	void markDirty();
	void onCredentialsFileDropped(const QString &localFile);
	void onAuthButtonClicked();
	void onClearAuthButtonClicked();
	void onApply();

private:
	void setupUi();

	void storeSettings();
	void restoreSettings();

	SettingsDialogGoogleOAuth2ClientCredentials
	parseGoogleOAuth2ClientCredentialsFromLocalFile(const QString &localFile);

	static Async::Task<void> runAuthFlow(QPointer<SettingsDialog> self);

	const std::shared_ptr<Store::AuthStore> authStore_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	// --- UI Components ---

	// 1. Main Structure
	QVBoxLayout *mainLayout_;
	QTabWidget *tabWidget_;

	// 2. General Tab
	QWidget *generalTab_;
	QVBoxLayout *generalTabLayout_;

	// 3. YouTube Tab
	QWidget *youTubeTab_;
	QVBoxLayout *youTubeTabLayout_;
	QLabel *helpLabel_;

	// 4. Credentials Group (YouTube)
	QGroupBox *credGroup_;
	QVBoxLayout *credLayout_;
	JsonDropArea *dropArea_;
	QLineEdit *clientIdDisplay_;
	QLineEdit *clientSecretDisplay_;

	// 5. Authorization Group (YouTube)
	QGroupBox *authGroup_;
	QVBoxLayout *authLayout_;
	QPushButton *authButton_;
	QPushButton *clearAuthButton_;
	QLabel *statusLabel_;

	// 6. Stream Settings Group (YouTube)
	QGroupBox *keyGroup_;
	QVBoxLayout *keyLayout_;
	QLabel *streamKeyLabelA_;
	QComboBox *streamKeyComboA_;
	QLabel *streamKeyLabelB_;
	QComboBox *streamKeyComboB_;

	// 7. Dialog Buttons
	QDialogButtonBox *buttonBox_;
	QPushButton *applyButton_;

	std::shared_ptr<GoogleAuth::GoogleOAuth2Flow> googleOAuth2Flow_{};
	Async::Task<void> currentAuthFlowTask_{nullptr};
	jthread_ns::jthread currentAuthTaskWorkerThread_;
	std::shared_ptr<GoogleOAuth2FlowCallbackServer> currentCallbackServer_{};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
