/*
 * Live Stream Segmenter - UI Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include "SettingsDialog.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThreadPool>
#include <QUrl>
#include <QUrlQuery>

#include <EventScriptingContext.hpp>
#include <GoogleAuthManager.hpp>
#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <Task.hpp>
#include <YouTubeApiClient.hpp>

#include "fmt_qstring_formatter.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

namespace {

struct ResumeOnGlobalQThreadPool {
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h)
	{
		QThreadPool::globalInstance()->start([h]() { h.resume(); });
	}
	void await_resume() {}
};

struct ResumeOnQtMainThread {
	QPointer<QObject> context;
	explicit ResumeOnQtMainThread(QObject *c) : context(c) {}

	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h)
	{
		if (context) {
			QMetaObject::invokeMethod(context, [h]() { h.resume(); }, Qt::QueuedConnection);
		}
	}
	void await_resume() {}
};

} // anonymous namespace

SettingsDialog::SettingsDialog(std::shared_ptr<Scripting::ScriptingRuntime> runtime,
			       std::shared_ptr<Store::AuthStore> authStore,
			       std::shared_ptr<Store::EventHandlerStore> eventHandlerStore,
			       std::shared_ptr<Store::YouTubeStore> youTubeStore,
			       std::shared_ptr<const Logger::ILogger> logger, QWidget *parent)
	: QDialog(parent),
	  runtime_(runtime ? std::move(runtime)
			   : throw std::invalid_argument("RuntimeIsNullError(SettingsDialog::SettingsDialog)")),
	  authStore_(authStore ? std::move(authStore)
			       : throw std::invalid_argument("AuthStoreIsNullError(SettingsDialog::SettingsDialog)")),
	  eventHandlerStore_(eventHandlerStore
				     ? std::move(eventHandlerStore)
				     : throw std::invalid_argument(
					       "EventHandlerStoreIsNullError(SettingsDialog::SettingsDialog)")),
	  youTubeStore_(youTubeStore ? std::move(youTubeStore)
				     : throw std::invalid_argument(
					       "YouTubeStoreIsNullError(SettingsDialog::SettingsDialog)")),
	  logger_(logger ? std::move(logger)
			 : throw std::invalid_argument("LoggerIsNullError(SettingsDialog::SettingsDialog)")),

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

	  // 7. Script Tab
	  scriptTab_(new QWidget(this)),
	  scriptTabLayout_(new QVBoxLayout(scriptTab_)),
	  scriptHelpLabel_(new QLabel(this)),
	  scriptEditor_(new QPlainTextEdit(this)),
	  scriptFunctionCombo_(new QComboBox(this)),
	  runScriptButton_(new QPushButton(this)),

	  // 8. LocalStorage Tab
	  localStorageTab_(new QWidget(this)),
	  localStorageTabLayout_(new QVBoxLayout(localStorageTab_)),
	  localStorageHelpLabel_(new QLabel(this)),
	  localStorageTable_(new QTableWidget(this)),
	  localStorageButtonsWidget_(new QWidget(this)),
	  localStorageButtonsLayout_(new QVBoxLayout(localStorageButtonsWidget_)),
	  addLocalStorageButton_(new QPushButton(this)),
	  editLocalStorageButton_(new QPushButton(this)),
	  deleteLocalStorageButton_(new QPushButton(this)),

	  // 9. Dialog Buttons
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

	connect(runScriptButton_, &QPushButton::clicked, this, &SettingsDialog::onRunScriptClicked);

	connect(addLocalStorageButton_, &QPushButton::clicked, this, &SettingsDialog::onAddLocalStorageItem);
	connect(editLocalStorageButton_, &QPushButton::clicked, this, &SettingsDialog::onEditLocalStorageItem);
	connect(deleteLocalStorageButton_, &QPushButton::clicked, this, &SettingsDialog::onDeleteLocalStorageItem);
	connect(localStorageTable_, &QTableWidget::cellDoubleClicked, this,
		&SettingsDialog::onLocalStorageTableDoubleClicked);

	connect(buttonBox_, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
	connect(buttonBox_, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
	connect(buttonBox_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);

	currentFetchStreamKeysTask_ = fetchStreamKeys();
	currentFetchStreamKeysTask_.start();

	loadLocalStorageData();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::accept()
{
	saveSettings();
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
		logger_->warn("FlowAlreadyRunning");
		return;
	}

	authButton_->setEnabled(false);
	statusLabel_->setText(tr("Waiting for browser..."));

	currentAuthFlowTask_ = runAuthFlow(this);
	currentAuthFlowTask_.start();
}

void SettingsDialog::onClearAuthButtonClicked()
{
	authStore_->setGoogleTokenState({});
	statusLabel_->setText(tr("Cleared to be Unauthorized"));
	logger_->info("TokenCleared");
}

void SettingsDialog::onCredentialsFileDropped(const QString &localFile)
{
	try {
		SettingsDialogGoogleOAuth2ClientCredentials credentials =
			parseGoogleOAuth2ClientCredentialsFromLocalFile(localFile);
		clientIdDisplay_->setText(credentials.client_id);
		clientSecretDisplay_->setText(credentials.client_secret);
	} catch (const std::exception &e) {
		logger_->error("ParseDroppedCredentialsFileError", {{"exception", e.what()}});

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
	saveSettings();
	if (authStore_->getGoogleTokenState().isAuthorized()) {
		statusLabel_->setText(tr("Authorized (Saved)"));
	} else {
		statusLabel_->setText(tr("Unauthorized"));
	}
	applyButton_->setEnabled(false);
}

void SettingsDialog::onRunScriptClicked()
try {
	const std::shared_ptr<const Logger::ILogger> logger = logger_;

	std::shared_ptr<JSContext> ctx = runtime_->createContextRaw();
	Scripting::EventScriptingContext context(runtime_, ctx, logger);
	Scripting::ScriptingDatabase database(runtime_, ctx, logger, eventHandlerStore_->getEventHandlerDatabasePath(),
					      true);
	context.setupContext();
	database.setupContext();
	context.setupLocalStorage();

	const std::string scriptContent = scriptEditor_->toPlainText().toStdString();
	context.loadEventHandler(scriptContent.c_str());

	QString selectedFunction = scriptFunctionCombo_->currentText();
	std::string functionName = selectedFunction.toStdString();
	std::string result = context.executeFunction(functionName.c_str(), R"({})");

	QMessageBox::information(this, tr("Script Result"), QString::fromStdString(result));

	loadLocalStorageData();
} catch (const std::exception &e) {
	logger_->error("RunScriptError", {{"exception", e.what()}});

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Error"));
	msgBox.setText(tr("Failed to run the event handler script."));
	msgBox.setInformativeText(tr("Please ensure the script is valid and does not contain errors."));
	msgBox.setDetailedText(QString::fromStdString(e.what()));
	msgBox.exec();
} catch (...) {
	logger_->error("RunScriptUnknownError");

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Error"));
	msgBox.setText(tr("An unknown error occurred while running the event handler script."));
	msgBox.exec();
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
	streamKeyComboA_->setPlaceholderText(tr("-"));
	streamKeyComboA_->setEnabled(false);
	streamKeyComboA_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	streamKeyFormLayout->addRow(streamKeyLabelA_, streamKeyComboA_);

	// Key B
	streamKeyLabelB_->setText(tr("Stream Key B"));
	streamKeyComboB_->setPlaceholderText(tr("-"));
	streamKeyComboB_->setEnabled(false);
	streamKeyComboB_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	streamKeyFormLayout->addRow(streamKeyLabelB_, streamKeyComboB_);

	keyLayout_->addLayout(streamKeyFormLayout);
	youTubeTabLayout_->addWidget(keyGroup_);

	youTubeTabLayout_->addStretch();

	// --- Script Tab Config ---
	scriptTabLayout_->setSpacing(8);
	scriptTabLayout_->setContentsMargins(16, 16, 16, 16);

	scriptHelpLabel_->setText(tr("Enter a JavaScript function to generate stream metadata.\n"
				     "The script should return a JSON object containing 'title' and 'settings'."));
	scriptHelpLabel_->setWordWrap(true);
	scriptTabLayout_->addWidget(scriptHelpLabel_);

	const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	scriptEditor_->setFont(fixedFont);

	scriptFunctionCombo_->addItem("onCreateYouTubeLiveBroadcast");
	scriptFunctionCombo_->addItem("onSetThumbnailOnCreatedYouTubeLiveBroadcast");
	scriptTabLayout_->addWidget(scriptFunctionCombo_);

	scriptEditor_->setPlaceholderText(tr("// Example Script\n"
					     "function generate(context) {\n"
					     "    return {\n"
					     "        title: \"My Stream \" + context.date,\n"
					     "        settings: {}\n"
					     "    };\n"
					     "}"));

	scriptEditor_->setPlainText(QString::fromStdString(eventHandlerStore_->getEventHandlerScript()));

	scriptTabLayout_->addWidget(scriptEditor_);
	runScriptButton_->setText(tr("Run Script (Test)"));
	scriptTabLayout_->addWidget(runScriptButton_);

	// --- LocalStorage Tab Config ---
	localStorageTabLayout_->setSpacing(8);
	localStorageTabLayout_->setContentsMargins(16, 16, 16, 16);

	localStorageHelpLabel_->setText(
		tr("View and edit localStorage key-value pairs used by event handler scripts."));
	localStorageHelpLabel_->setWordWrap(true);
	localStorageTabLayout_->addWidget(localStorageHelpLabel_);

	localStorageTable_->setColumnCount(2);
	localStorageTable_->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
	localStorageTable_->horizontalHeader()->setStretchLastSection(true);
	localStorageTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
	localStorageTable_->setSelectionMode(QAbstractItemView::SingleSelection);
	localStorageTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	localStorageTabLayout_->addWidget(localStorageTable_);

	localStorageButtonsLayout_->setSpacing(8);
	localStorageButtonsLayout_->setContentsMargins(0, 0, 0, 0);

	addLocalStorageButton_->setText(tr("Add"));
	editLocalStorageButton_->setText(tr("Edit"));
	deleteLocalStorageButton_->setText(tr("Delete"));

	localStorageButtonsLayout_->addWidget(addLocalStorageButton_);
	localStorageButtonsLayout_->addWidget(editLocalStorageButton_);
	localStorageButtonsLayout_->addWidget(deleteLocalStorageButton_);
	localStorageButtonsLayout_->addStretch();

	localStorageTabLayout_->addWidget(localStorageButtonsWidget_);

	// --- Finalize Tabs ---
	tabWidget_->addTab(generalTab_, tr("General"));
	tabWidget_->addTab(youTubeTab_, tr("YouTube"));
	tabWidget_->addTab(scriptTab_, tr("Script"));
	tabWidget_->addTab(localStorageTab_, tr("LocalStorage"));
	mainLayout_->addWidget(tabWidget_);

	tabWidget_->setCurrentWidget(youTubeTab_);

	// --- Dialog Buttons ---
	applyButton_->setEnabled(false);
	mainLayout_->addWidget(buttonBox_);

	// Window Sizing
	mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);

	resize(500, 700);
}

void SettingsDialog::saveSettings()
{
	// Save AuthStore
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials;
	googleOAuth2ClientCredentials.client_id = clientIdDisplay_->text().toStdString();
	googleOAuth2ClientCredentials.client_secret = clientSecretDisplay_->text().toStdString();
	authStore_->setGoogleOAuth2ClientCredentials(googleOAuth2ClientCredentials);

	authStore_->saveAuthStore();

	// Save EventHandlerStore
	eventHandlerStore_->setEventHandlerScript(scriptEditor_->toPlainText().toStdString());
	eventHandlerStore_->save();

	// Save YouTubeStore
	int streamKeyAIndex = streamKeyComboA_->currentIndex();
	if (streamKeyAIndex >= 0) {
		youTubeStore_->setStreamKeyA(streamKeys_[streamKeyAIndex]);
	} else {
		youTubeStore_->setStreamKeyA({});
	}

	int streamKeyBIndex = streamKeyComboB_->currentIndex();
	if (streamKeyBIndex >= 0) {
		youTubeStore_->setStreamKeyB(streamKeys_[streamKeyBIndex]);
	} else {
		youTubeStore_->setStreamKeyB({});
	}

	youTubeStore_->saveYouTubeStore();

	// Save LocalStorage data
	saveLocalStorageData();
}

inline SettingsDialogGoogleOAuth2ClientCredentials
SettingsDialog::parseGoogleOAuth2ClientCredentialsFromLocalFile(const QString &localFile)
{
	QFile file(localFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		logger_->error("FailedToOpenDroppedCredentialsFile", {{"file", localFile.toStdString()}});
		throw std::runtime_error("FileOpenError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	file.close();

	if (parseError.error != QJsonParseError::NoError) {
		logger_->error("FailedToParseDroppedCredentialsFile",
			       {{"file", localFile.toStdString()}, {"error", parseError.errorString().toStdString()}});
		throw std::runtime_error("JsonParseError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	if (!doc.isObject()) {
		logger_->error("DroppedCredentialsFileNotJsonObject", {{"file", localFile.toStdString()}});
		throw std::runtime_error("RootIsNotObjectError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonObject root = doc.object();
	if (!root.contains("installed") || !root["installed"].isObject()) {
		logger_->error("DroppedCredentialsFileMissingInstalledObject", {{"file", localFile.toStdString()}});
		throw std::runtime_error(
			"InstalledObjectMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	QJsonObject installed = root["installed"].toObject();
	if (!installed.contains("client_id") || !installed["client_id"].isString()) {
		logger_->error("DroppedCredentialsFileMissingClientId", {{"file", localFile.toStdString()}});
		throw std::runtime_error("ClientIdMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}
	if (!installed.contains("client_secret") || !installed["client_secret"].isString()) {
		logger_->error("DroppedCredentialsFileMissingClientSecret", {{"file", localFile.toStdString()}});
		throw std::runtime_error("ClientSecretMissingError(parseGoogleOAuth2ClientCredentialsFromLocalFile)");
	}

	SettingsDialogGoogleOAuth2ClientCredentials credentials;
	credentials.client_id = installed["client_id"].toString();
	credentials.client_secret = installed["client_secret"].toString();
	return credentials;
}

Async::Task<void> SettingsDialog::runAuthFlow(QPointer<SettingsDialog> self)
{
	if (!self)
		co_return;

	auto logger = self->logger_;

	logger->info("OAuth2FlowStart");
	GoogleAuth::GoogleOAuth2ClientCredentials clientCredentials;
	clientCredentials.client_id = self->clientIdDisplay_->text().toStdString();
	clientCredentials.client_secret = self->clientSecretDisplay_->text().toStdString();

	self->googleOAuth2Flow_ = std::make_shared<GoogleAuth::GoogleOAuth2Flow>(
		clientCredentials, "https://www.googleapis.com/auth/youtube.force-ssl", logger);

	auto flow = self->googleOAuth2Flow_;
	std::optional<GoogleAuth::GoogleAuthResponse> result = std::nullopt;

	try {
		self->currentCallbackServer_ = std::make_shared<GoogleOAuth2FlowCallbackServer>();

		uint16_t port = self->currentCallbackServer_->serverPort();
		std::string redirectUri = fmt::format("http://127.0.0.1:{}/callback", port);
		std::string authUrl = flow->getAuthorizationUrl(redirectUri);
		logger->info("OAuth2OpenAuthUrl", {{"url", authUrl}});

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

		if (!self)
			co_return;

		if (code.empty()) {
			throw std::runtime_error("Authorization code was empty.");
		}

		co_await ResumeOnGlobalQThreadPool{};

		result = flow->exchangeCodeForToken(code, redirectUri);
	} catch (const std::exception &e) {
		logger->error("OAuthFlowFailed", {{"exception", e.what()}});
	}

	co_await ResumeOnQtMainThread{self};

	if (!self)
		co_return;

	self->currentCallbackServer_.reset();
	self->googleOAuth2Flow_.reset();

	if (result.has_value()) {
		logger->info("OAuth2AuthSuccess");
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

	self->currentFetchStreamKeysTask_ = self->fetchStreamKeys();
	self->currentFetchStreamKeysTask_.start();
}

Async::Task<void> SettingsDialog::fetchStreamKeys()
{
	std::shared_ptr<const Logger::ILogger> logger = logger_;

	co_await ResumeOnGlobalQThreadPool{};
	try {
		const GoogleAuth::GoogleAuthManager authManager(authStore_->getGoogleOAuth2ClientCredentials(),
								logger_);
		const GoogleAuth::GoogleTokenState tokenState = authStore_->getGoogleTokenState();

		std::string accessToken;
		if (tokenState.isAuthorized()) {
			if (tokenState.isAccessTokenFresh()) {
				logger->info("YouTubeAccessTokenFresh");
				accessToken = tokenState.access_token;
			} else {
				logger->info("YouTubeAccessTokenNotFresh");
				GoogleAuth::GoogleAuthResponse freshAuthResponse =
					authManager.fetchFreshAuthResponse(tokenState.refresh_token);
				GoogleAuth::GoogleTokenState newTokenState =
					tokenState.withUpdatedAuthResponse(freshAuthResponse);
				authStore_->setGoogleTokenState(newTokenState);
				accessToken = freshAuthResponse.access_token;
				logger->info("YouTubeAccessTokenFetched");
			}
		}

		YouTubeApi::YouTubeApiClient client(logger);
		std::vector<YouTubeApi::YouTubeLiveStream> streamKeys = client.listStreamKeys(accessToken);

		co_await ResumeOnQtMainThread{this};

		streamKeyComboA_->clear();
		streamKeyComboB_->clear();

		streamKeys_ = std::move(streamKeys);

		YouTubeApi::YouTubeLiveStream currentStreamKeyA = youTubeStore_->getStreamKeyA();
		YouTubeApi::YouTubeLiveStream currentStreamKeyB = youTubeStore_->getStreamKeyB();


		logger_->info("CurrentStreamKeys",
			      {{"streamKeyA_id", currentStreamKeyA.id}, {"streamKeyB_id", currentStreamKeyB.id}});
		for (int i = 0; i < static_cast<int>(streamKeys_.size()); ++i) {
			const YouTubeApi::YouTubeLiveStream &key = streamKeys_[i];

			QString displayText = QString::fromStdString(
				fmt::format("{} ({} - {})", key.snippet.title, key.cdn.resolution, key.cdn.frameRate));
			streamKeyComboA_->addItem(displayText, QString::fromStdString(key.id));
			streamKeyComboB_->addItem(displayText, QString::fromStdString(key.id));

			logger->info("StreamKeyListed",
				     {{"id", key.id}, {"title", key.snippet.title}, {"resolution", key.cdn.resolution},
				      {"frameRate", key.cdn.frameRate}});
			if (currentStreamKeyA.id == key.id) {
				streamKeyComboA_->setCurrentIndex(i);
			}

			if (currentStreamKeyB.id == key.id) {
				streamKeyComboB_->setCurrentIndex(i);
			}
		}

		streamKeyComboA_->setEnabled(true);
		streamKeyComboB_->setEnabled(true);

		connect(streamKeyComboA_, &QComboBox::currentTextChanged, this, &SettingsDialog::markDirty,
			Qt::UniqueConnection);
		connect(streamKeyComboB_, &QComboBox::currentTextChanged, this, &SettingsDialog::markDirty,
			Qt::UniqueConnection);

		connect(scriptEditor_, &QPlainTextEdit::textChanged, this, &SettingsDialog::markDirty,
			Qt::UniqueConnection);
	} catch (const std::exception &e) {
		logger->error("FetchStreamKeysFailed", {{"exception", e.what()}});
	}
}

void SettingsDialog::loadLocalStorageData()
try {
	std::shared_ptr<const Logger::ILogger> logger = logger_;

	std::filesystem::path dbPath = eventHandlerStore_->getEventHandlerDatabasePath();
	if (dbPath.empty()) {
		logger->warn("DatabasePathEmpty");
		return;
	}

	// Check if database file exists
	if (!std::filesystem::exists(dbPath)) {
		logger->info("DatabaseFileNotFound", {{"path", dbPath.string()}});
		return;
	}

	std::shared_ptr<JSContext> ctx = runtime_->createContextRaw();
	Scripting::ScriptingDatabase database(runtime_, ctx, logger, dbPath, false);
	database.setupContext();

	// Get the db object from global scope
	JSValue globalObj = JS_GetGlobalObject(ctx.get());
	JSValue dbObj = JS_GetPropertyStr(ctx.get(), globalObj, "db");

	// Query the localStorage table
	JSValue sqlStr = JS_NewString(ctx.get(), "SELECT key, value FROM __sys_local_storage");
	JSValue args[] = {sqlStr};
	JSValue queryResultRaw = Scripting::ScriptingDatabase::query(ctx.get(), dbObj, 1, args);
	Scripting::ScopedJSValue queryResult(ctx.get(), queryResultRaw);

	JS_FreeValue(ctx.get(), sqlStr);
	Scripting::ScopedJSValue scopedDbObj(ctx.get(), dbObj);
	Scripting::ScopedJSValue scopedGlobalObj(ctx.get(), globalObj);

	if (JS_IsException(queryResult.get())) {
		logger->warn("LocalStorageQueryFailed");
		return;
	}

	// Clear existing table data
	localStorageTable_->setRowCount(0);

	// Parse the result array
	if (JS_IsArray(queryResult.get())) {
		JSValue lengthVal = JS_GetPropertyStr(ctx.get(), queryResult.get(), "length");
		int32_t length = 0;
		JS_ToInt32(ctx.get(), &length, lengthVal);
		JS_FreeValue(ctx.get(), lengthVal);

		for (int32_t i = 0; i < length; ++i) {
			JSValue rowVal = JS_GetPropertyUint32(ctx.get(), queryResult.get(), i);

			JSValue keyVal = JS_GetPropertyStr(ctx.get(), rowVal, "key");
			JSValue valueVal = JS_GetPropertyStr(ctx.get(), rowVal, "value");

			Scripting::ScopedJSString keyStr(ctx.get(), JS_ToCString(ctx.get(), keyVal));
			Scripting::ScopedJSString valueStr(ctx.get(), JS_ToCString(ctx.get(), valueVal));

			if (keyStr && valueStr) {
				int row = localStorageTable_->rowCount();
				localStorageTable_->insertRow(row);
				localStorageTable_->setItem(row, 0,
							    new QTableWidgetItem(QString::fromUtf8(keyStr.get())));
				localStorageTable_->setItem(row, 1,
							    new QTableWidgetItem(QString::fromUtf8(valueStr.get())));
			}

			JS_FreeValue(ctx.get(), keyVal);
			JS_FreeValue(ctx.get(), valueVal);
			JS_FreeValue(ctx.get(), rowVal);
		}
	}
} catch (const std::exception &e) {
	logger_->error("LoadLocalStorageError", {{"exception", e.what()}});

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setWindowTitle(tr("Warning"));
	msgBox.setText(tr("Failed to load localStorage data."));
	msgBox.setInformativeText(tr("The database might be corrupted or not yet created."));
	msgBox.setDetailedText(QString::fromStdString(e.what()));
	msgBox.exec();
} catch (...) {
	logger_->error("LoadLocalStorageUnknownError");
}

void SettingsDialog::saveLocalStorageData()
try {
	std::shared_ptr<const Logger::ILogger> logger = logger_;

	std::filesystem::path dbPath = eventHandlerStore_->getEventHandlerDatabasePath();
	if (dbPath.empty()) {
		logger->error("DatabasePathEmpty");
		return;
	}

	std::shared_ptr<JSContext> ctx = runtime_->createContextRaw();
	Scripting::ScriptingDatabase database(runtime_, ctx, logger, dbPath, true);
	database.setupContext();

	// Get the db object from global scope
	JSValue globalObj = JS_GetGlobalObject(ctx.get());
	JSValue dbObj = JS_GetPropertyStr(ctx.get(), globalObj, "db");

	// Ensure the localStorage table exists
	JSValue createTableSql = JS_NewString(
		ctx.get(), "CREATE TABLE IF NOT EXISTS __sys_local_storage (key TEXT PRIMARY KEY, value TEXT)");
	JSValue createTableArgs[] = {createTableSql};
	JSValue createTableResultRaw = Scripting::ScriptingDatabase::execute(ctx.get(), dbObj, 1, createTableArgs);
	Scripting::ScopedJSValue createTableResult(ctx.get(), createTableResultRaw);

	JS_FreeValue(ctx.get(), createTableSql);

	if (JS_IsException(createTableResult.get())) {
		JS_FreeValue(ctx.get(), dbObj);
		JS_FreeValue(ctx.get(), globalObj);
		logger->error("LocalStorageCreateTableFailed");
		throw std::runtime_error("Failed to create localStorage table");
	}

	// Clear existing localStorage table
	JSValue deleteSql = JS_NewString(ctx.get(), "DELETE FROM __sys_local_storage");
	JSValue deleteArgs[] = {deleteSql};
	JSValue deleteResultRaw = Scripting::ScriptingDatabase::execute(ctx.get(), dbObj, 1, deleteArgs);
	Scripting::ScopedJSValue deleteResult(ctx.get(), deleteResultRaw);

	JS_FreeValue(ctx.get(), deleteSql);

	if (JS_IsException(deleteResult.get())) {
		JS_FreeValue(ctx.get(), dbObj);
		JS_FreeValue(ctx.get(), globalObj);
		logger->error("LocalStorageDeleteFailed");
		throw std::runtime_error("Failed to clear localStorage table");
	}

	// Insert all items from the table
	for (int row = 0; row < localStorageTable_->rowCount(); ++row) {
		QTableWidgetItem *keyItem = localStorageTable_->item(row, 0);
		QTableWidgetItem *valueItem = localStorageTable_->item(row, 1);

		if (keyItem && valueItem) {
			std::string key = keyItem->text().toStdString();
			std::string value = valueItem->text().toStdString();

			JSValue insertArgs[] = {
				JS_NewString(ctx.get(), "INSERT INTO __sys_local_storage (key, value) VALUES (?, ?)"),
				JS_NewString(ctx.get(), key.c_str()), JS_NewString(ctx.get(), value.c_str())};

			JSValue insertResultRaw =
				Scripting::ScriptingDatabase::execute(ctx.get(), dbObj, 3, insertArgs);
			Scripting::ScopedJSValue insertResult(ctx.get(), insertResultRaw);

			JS_FreeValue(ctx.get(), insertArgs[0]);
			JS_FreeValue(ctx.get(), insertArgs[1]);
			JS_FreeValue(ctx.get(), insertArgs[2]);

			if (JS_IsException(insertResult.get())) {
				logger->error("LocalStorageInsertFailed", {{"key", key}});
			}
		}
	}

	JS_FreeValue(ctx.get(), dbObj);
	JS_FreeValue(ctx.get(), globalObj);

	logger->info("LocalStorageSaved");
} catch (const std::exception &e) {
	logger_->error("SaveLocalStorageError", {{"exception", e.what()}});

	QMessageBox msgBox(this);
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Error"));
	msgBox.setText(tr("Failed to save localStorage data."));
	msgBox.setDetailedText(QString::fromStdString(e.what()));
	msgBox.exec();
} catch (...) {
	logger_->error("SaveLocalStorageUnknownError");
}

void SettingsDialog::onAddLocalStorageItem()
{
	bool keyOk;
	QString key =
		QInputDialog::getText(this, tr("Add LocalStorage Item"), tr("Key:"), QLineEdit::Normal, "", &keyOk);

	if (!keyOk || key.isEmpty()) {
		return;
	}

	// Check if key already exists
	for (int row = 0; row < localStorageTable_->rowCount(); ++row) {
		QTableWidgetItem *keyItem = localStorageTable_->item(row, 0);
		if (keyItem && keyItem->text() == key) {
			QMessageBox::warning(this, tr("Duplicate Key"),
					     tr("A key with this name already exists. Please use a different key."));
			return;
		}
	}

	bool valueOk;
	QString value =
		QInputDialog::getText(this, tr("Add LocalStorage Item"), tr("Value:"), QLineEdit::Normal, "", &valueOk);

	if (!valueOk) {
		return;
	}

	int row = localStorageTable_->rowCount();
	localStorageTable_->insertRow(row);
	localStorageTable_->setItem(row, 0, new QTableWidgetItem(key));
	localStorageTable_->setItem(row, 1, new QTableWidgetItem(value));

	markDirty();
}

void SettingsDialog::onEditLocalStorageItem()
{
	int currentRow = localStorageTable_->currentRow();
	if (currentRow < 0) {
		QMessageBox::information(this, tr("No Selection"), tr("Please select an item to edit."));
		return;
	}

	QTableWidgetItem *keyItem = localStorageTable_->item(currentRow, 0);
	QTableWidgetItem *valueItem = localStorageTable_->item(currentRow, 1);

	if (!keyItem || !valueItem) {
		return;
	}

	bool valueOk;
	QString newValue = QInputDialog::getText(this, tr("Edit LocalStorage Item"),
						 tr("Value for '%1':").arg(keyItem->text()), QLineEdit::Normal,
						 valueItem->text(), &valueOk);

	if (!valueOk) {
		return;
	}

	valueItem->setText(newValue);
	markDirty();
}

void SettingsDialog::onDeleteLocalStorageItem()
{
	int currentRow = localStorageTable_->currentRow();
	if (currentRow < 0) {
		QMessageBox::information(this, tr("No Selection"), tr("Please select an item to delete."));
		return;
	}

	QTableWidgetItem *keyItem = localStorageTable_->item(currentRow, 0);
	if (!keyItem) {
		return;
	}

	QMessageBox::StandardButton reply = QMessageBox::question(
		this, tr("Delete Item"),
		tr("Are you sure you want to delete the item with key '%1'?").arg(keyItem->text()),
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		localStorageTable_->removeRow(currentRow);
		markDirty();
	}
}

void SettingsDialog::onLocalStorageTableDoubleClicked(int row, int)
{
	if (row >= 0) {
		onEditLocalStorageItem();
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
