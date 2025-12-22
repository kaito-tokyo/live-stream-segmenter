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

#include <QDesktopServices>
#include <QFile>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <Task.hpp>
#include <TaskStorage.hpp>

#include "fmt_qstring_formatter.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

namespace {

struct ResumeOnJThread {
	jthread_ns::jthread &threadStorage;

	bool await_ready() { return false; }

	void await_suspend(std::coroutine_handle<> h)
	{
		threadStorage = jthread_ns::jthread([h] { h.resume(); });
	}

	void await_resume() {}
};

struct ResumeOnMainThread {
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h)
	{
		QMetaObject::invokeMethod(qApp, [h]() { h.resume(); }, Qt::QueuedConnection);
	}
	void await_resume() {}
};

} // namespace

SettingsDialog::SettingsDialog(std::shared_ptr<Store::AuthStore> authStore,
			       std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
	: QDialog(parent),
	  authStore_(std::move(authStore)),
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
	  clearAuthButton_(new QPushButton(this)),
	  statusLabel_(new QLabel(this)),

	  // 6. Stream Settings Group
	  keyGroup_(new QGroupBox(this)),
	  keyLayout_(new QVBoxLayout(keyGroup_)),
	  streamKeyLabelA_(new QLabel(this)),
	  streamKeyComboA_(new QComboBox(this)),
	  streamKeyLabelB_(new QLabel(this)),
	  streamKeyComboB_(new QComboBox(this)),

	  // 7. Dialog Buttons
	  buttonBox_(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
					  this)),
	  applyButton_(buttonBox_->button(QDialogButtonBox::Apply))
{
	setupUi();

	connect(dropArea_, &JsonDropArea::jsonFileDropped, this, &SettingsDialog::onCredentialsFileDropped);

	connect(clientIdDisplay_, &QLineEdit::textChanged, this, &SettingsDialog::markDirty);
	connect(clientSecretDisplay_, &QLineEdit::textChanged, this, &SettingsDialog::markDirty);

	connect(authButton_, &QPushButton::clicked, this, &SettingsDialog::onAuthButtonClicked);
	connect(clearAuthButton_, &QPushButton::clicked, this, &SettingsDialog::onClearAuthButtonClicked);

	connect(buttonBox_, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
	connect(buttonBox_, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
	connect(buttonBox_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::accept()
{
	storeSettings();
	QDialog::accept();
}

void SettingsDialog::markDirty()
{
	applyButton_->setEnabled(true);
}

void SettingsDialog::onAuthButtonClicked()
{
	if (clientIdDisplay_->text().isEmpty() || clientSecretDisplay_->text().isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Client ID and Client Secret must be provided before requesting authorization."));
		return;
	}

	if (googleOAuth2Flow_) {
		logger_->warn("OAuth2 flow is already in progress.");
		return;
	}

	authButton_->setEnabled(false);
	statusLabel_->setText(tr("Waiting for browser..."));

	currentAuthFlowTask_ = runAuthFlow(std::allocator_arg, currentAuthFlowTaskStorage_, this);
	currentAuthFlowTask_.start();
}

void SettingsDialog::onClearAuthButtonClicked()
{
	authStore_->clearGoogleTokenState();
	statusLabel_->setText(tr("Cleared to be Unauthorized"));
	logger_->info("Cleared stored Google OAuth2 token.");
}

void SettingsDialog::onCredentialsFileDropped(const QString &localFile)
{
	try {
		SettingsDialogGoogleOAuth2ClientCredentials credentials =
			parseGoogleOAuth2ClientCredentialsFromLocalFile(localFile);
		clientIdDisplay_->setText(credentials.client_id);
		clientSecretDisplay_->setText(credentials.client_secret);
	} catch (const std::exception &e) {
		logger_->logException(e, "Error parsing dropped credentials file");

		QMessageBox msgBox(this);
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle(tr("Error"));
		msgBox.setText(tr("Failed to load credentials from the dropped file."));
		msgBox.setInformativeText(tr("Please ensure the file is a valid Google OAuth2 credentials JSON file."));
		msgBox.setDetailedText(QString::fromStdString(e.what()));
		msgBox.exec();
	}
}

void SettingsDialog::onApply()
{
	storeSettings();
	if (authStore_->getGoogleTokenState().isAuthorized()) {
		statusLabel_->setText(tr("Authorized (Saved)"));
	} else {
		statusLabel_->setText(tr("Unauthorized"));
	}
	applyButton_->setEnabled(false);
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

	auto clientCredentials = authStore_->getGoogleOAuth2ClientCredentials();

	clientIdDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	clientIdDisplay_->setText(QString::fromStdString(clientCredentials.client_id));
	detailsLayout->addRow(tr("Client ID"), clientIdDisplay_);

	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	clientSecretDisplay_->setText(QString::fromStdString(clientCredentials.client_secret));
	detailsLayout->addRow(tr("Client Secret"), clientSecretDisplay_);

	credLayout_->addLayout(detailsLayout);
	youTubeTabLayout_->addWidget(credGroup_);

	// --- 2. OAuth2 Authorization Group ---
	authGroup_->setTitle(tr("2. OAuth2 Authorization"));

	authButton_->setText(tr("Request Authorization"));

	clearAuthButton_->setText(tr("Clear Token"));

	statusLabel_->setAlignment(Qt::AlignCenter);

	if (authStore_->getGoogleTokenState().isAuthorized()) {
		statusLabel_->setText(tr("Authorized (Saved)"));
	} else {
		statusLabel_->setText(tr("Unauthorized"));
	}

	authLayout_->addWidget(authButton_);
	authLayout_->addWidget(clearAuthButton_);
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
	applyButton_->setEnabled(false);
	mainLayout_->addWidget(buttonBox_);

	// Window Sizing
	mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);

	resize(500, 700);
}

void SettingsDialog::storeSettings()
{
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials;
	googleOAuth2ClientCredentials.client_id = clientIdDisplay_->text().toStdString();
	googleOAuth2ClientCredentials.client_secret = clientSecretDisplay_->text().toStdString();
	authStore_->setGoogleOAuth2ClientCredentials(googleOAuth2ClientCredentials);

	authStore_->saveAuthStore();
}

inline SettingsDialogGoogleOAuth2ClientCredentials
SettingsDialog::parseGoogleOAuth2ClientCredentialsFromLocalFile(const QString &localFile)
{
	QFile file(localFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		logger_->error("Failed to open dropped credentials file: {}", localFile);
		throw std::runtime_error("FileOpenError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();

	if (parseError.error != QJsonParseError::NoError) {
		logger_->error("Failed to parse dropped credentials file: {}: {}", localFile, parseError.errorString());
		throw std::runtime_error("JsonParseError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	if (!doc.isObject()) {
		logger_->error("Dropped credentials file is not a valid JSON object: {}", localFile);
		throw std::runtime_error("RootIsNotObjectError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonObject root = doc.object();
	if (!root.contains("installed") || !root["installed"].isObject()) {
		logger_->error("Dropped credentials file does not contain 'installed' object: {}", localFile);
		throw std::runtime_error(
			"InstalledObjectMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonObject installed = root["installed"].toObject();
	if (!installed.contains("client_id") || !installed["client_id"].isString()) {
		logger_->error("Dropped credentials file is missing 'installed.client_id': {}", localFile);
		throw std::runtime_error("ClientIdMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}
	if (!installed.contains("client_secret") || !installed["client_secret"].isString()) {
		logger_->error("Dropped credentials file is missing 'installed.client_secret': {}", localFile);
		throw std::runtime_error("ClientSecretMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	SettingsDialogGoogleOAuth2ClientCredentials credentials;
	credentials.client_id = installed["client_id"].toString();
	credentials.client_secret = installed["client_secret"].toString();
	return credentials;
}

Async::Task<void> SettingsDialog::runAuthFlow(std::allocator_arg_t, Async::TaskStorage<> &,
					      QPointer<SettingsDialog> self)
{
	if (!self)
		co_return;

	auto logger = self->logger_;

	logger->info("Starting OAuth2 flow...");
	GoogleAuth::GoogleOAuth2ClientCredentials clientCredentials;
	clientCredentials.client_id = self->clientIdDisplay_->text().toStdString();
	clientCredentials.client_secret = self->clientSecretDisplay_->text().toStdString();

	self->googleOAuth2Flow_ = std::make_shared<GoogleAuth::GoogleOAuth2Flow>(
		clientCredentials, "https://www.googleapis.com/auth/youtube.readonly", logger);

	auto flow = self->googleOAuth2Flow_;
	std::optional<GoogleAuth::GoogleAuthResponse> result = std::nullopt;

	try {
		self->currentCallbackServer_ = std::make_shared<GoogleOAuth2FlowCallbackServer>();

		uint16_t port = self->currentCallbackServer_->serverPort();
		std::string redirectUri = fmt::format("http://127.0.0.1:{}/callback", port);
		std::string authUrl = flow->getAuthorizationUrl(redirectUri);
		logger->error("Opening authorization URL: {}", authUrl);

		QString qUrlStr = QString::fromStdString(authUrl);
		bool success = QDesktopServices::openUrl(QUrl(qUrlStr));

		if (!success) {
			QMessageBox msgBox(self);
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setWindowTitle(tr("Warning"));
			msgBox.setText(tr("Cannot open the authorization URL in the default browser."));
			msgBox.setInformativeText(tr("Please manually visit:\n<a href=\"%1\">%1</a>").arg(qUrlStr));
			msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
			msgBox.exec();
		}

		std::string code = co_await self->currentCallbackServer_->waitForCode();

		if (code.empty()) {
			throw std::runtime_error("Authorization code was empty.");
		}

		co_await ResumeOnJThread{self->currentAuthTaskWorkerThread_};

		result = flow->exchangeCodeForToken(code, redirectUri);
	} catch (const std::exception &e) {
		logger->logException(e, "OAuth flow failed");
	}

	co_await ResumeOnMainThread{};

	if (!self)
		co_return;

	self->currentCallbackServer_.reset();
	self->currentAuthTaskWorkerThread_ = jthread_ns::jthread();
	self->googleOAuth2Flow_.reset();

	if (result.has_value()) {
		logger->info("Authorization successful!");
		QMessageBox::information(self, tr("Success"), tr("Authorization successful!"));

		auto tokenState = GoogleAuth::GoogleTokenState().withUpdatedAuthResponse(result.value());
		self->authStore_->setGoogleTokenState(tokenState);
		self->statusLabel_->setText(tr("Authorized (Not Saved)"));
		self->markDirty();
	} else {
		self->statusLabel_->setText(tr("Authorization Failed"));
		QString msg = tr("Authorization failed.");
		QMessageBox::critical(self, tr("Error"), msg);
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
