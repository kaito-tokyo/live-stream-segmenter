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

#pragma once

#include <memory>

#include <QDialog>
#include <QPointer>

#include <ILogger.hpp>

#include <AuthStore.hpp>
#include <EventHandlerStore.hpp>
#include <GoogleOAuth2Flow.hpp>
#include <ScriptingRuntime.hpp>
#include <Task.hpp>
#include <YouTubeApiClient.hpp>
#include <YouTubeStore.hpp>
#include <YouTubeTypes.hpp>

#include "GoogleOAuth2FlowCallbackServer.hpp"
#include "JsonDropArea.hpp"

class QAbstractItemView;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QHeaderView;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QThreadPool;
class QVBoxLayout;
class QWidget;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

struct SettingsDialogGoogleOAuth2ClientCredentials {
	QString client_id;
	QString client_secret;
};

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	SettingsDialog(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
		       std::shared_ptr<Store::AuthStore> authStore,
		       std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
		       std::shared_ptr<Store::YouTubeStore> youTubeStore, std::shared_ptr<const Logger::ILogger> logger,
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
	void onRunScriptClicked();
	void onAddLocalStorageItem();
	void onEditLocalStorageItem();
	void onDeleteLocalStorageItem();
	void onLocalStorageTableDoubleClicked(int row, int column);
	void onCodeReceived(const QString &code, const QUrl &redirectUri);

private:
	void setupUi();

	void saveSettings();
	void restoreSettings();

	void loadLocalStorageData();
	void saveLocalStorageData();

	SettingsDialogGoogleOAuth2ClientCredentials
	parseGoogleOAuth2ClientCredentialsFromLocalFile(const QString &localFile);

	void startGoogleOAuth2Flow();
	void runAuthFlow();

	void fetchStreamKeys();

	const std::shared_ptr<Scripting::ScriptingRuntime> runtime_;
	const std::shared_ptr<Store::AuthStore> authStore_;
	const std::shared_ptr<Store::EventHandlerStore> eventHandlerStore_;
	const std::shared_ptr<Store::YouTubeStore> youTubeStore_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	const std::shared_ptr<CurlHelper::CurlHandle> curl_;
	const std::shared_ptr<YouTubeApi::YouTubeApiClient> youTubeApiClient_;

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

	// 7. Script Tab
	QWidget *scriptTab_;
	QVBoxLayout *scriptTabLayout_;
	QLabel *scriptHelpLabel_;
	QPlainTextEdit *scriptEditor_;
	QComboBox *scriptFunctionCombo_;
	QPushButton *runScriptButton_;

	// 8. LocalStorage Tab
	QWidget *localStorageTab_;
	QVBoxLayout *localStorageTabLayout_;
	QLabel *localStorageHelpLabel_;
	QTableWidget *localStorageTable_;
	QWidget *localStorageButtonsWidget_;
	QVBoxLayout *localStorageButtonsLayout_;
	QPushButton *addLocalStorageButton_;
	QPushButton *editLocalStorageButton_;
	QPushButton *deleteLocalStorageButton_;

	// 9. Dialog Buttons
	QDialogButtonBox *buttonBox_;
	QPushButton *applyButton_;

	std::vector<YouTubeApi::YouTubeLiveStream> streamKeys_;

	std::shared_ptr<GoogleAuth::GoogleOAuth2Flow> googleOAuth2Flow_;
	Async::Task<void> currentAuthFlowTask_ = {};
	Async::Task<void> currentFetchStreamKeysTask_ = {};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
