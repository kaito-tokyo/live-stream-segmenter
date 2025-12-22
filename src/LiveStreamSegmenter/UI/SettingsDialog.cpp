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

#include "fmt_qstring_formatter.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

namespace {

struct ResumeOnJThread {
	jthread_ns::jthread &threadStorage; // スレッドを保持する場所への参照

	bool await_ready() { return false; }

	void await_suspend(std::coroutine_handle<> h)
	{
		threadStorage = jthread_ns::jthread([h] { h.resume(); });
	}

	void await_resume() {}
};

// ★ クライアントが用意する「メインスレッドへの復帰」実装
struct ResumeOnMainThread {
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h)
	{
		// Qtのイベントループへ戻す
		QMetaObject::invokeMethod(qApp, [h]() { h.resume(); }, Qt::QueuedConnection);
	}
	void await_resume() {}
};

// コルーチンとQtのシグナルをつなぐための使い捨てワーカー
class AuthCallbackReceiver : public QObject {
	Q_OBJECT
public:
	AuthCallbackReceiver(uint16_t port, std::coroutine_handle<> h, std::string &resultRef)
		: handle_(h),
		  resultRef_(resultRef)
	{
		server_ = new QTcpServer(this);
		connect(server_, &QTcpServer::newConnection, this, &AuthCallbackReceiver::onNewConnection);

		if (!server_->listen(QHostAddress::LocalHost, port)) {
			// ポートが使えないなどのエラー時は空文字で即座に再開
			qWarning() << "Failed to start local auth server on port" << port;
			cleanupAndResume();
		}
	}

private slots:
	void onNewConnection()
	{
		QTcpSocket *socket = server_->nextPendingConnection();
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { handleReadyRead(socket); });
		// 切断時の自動削除は手動で行うため設定しない、または親関係で管理
	}

	void handleReadyRead(QTcpSocket *socket)
	{
		// データ読み込み
		QByteArray data = socket->readAll();
		QString request = QString::fromUtf8(data);

		// 簡易的なHTTPリクエストラインのパース: "GET /callback?code=... HTTP/1.1"
		// 正規表現で最初の行からパスを抽出
		static const QRegularExpression re("^GET\\s+(\\S+)\\s+HTTP");
		QRegularExpressionMatch match = re.match(request);

		if (match.hasMatch()) {
			QString pathAndQuery = match.captured(1);
			QUrl url(pathAndQuery);
			QUrlQuery query(url);

			if (query.hasQueryItem("code")) {
				// 成功: コードを取得
				resultRef_ = query.queryItemValue("code").toStdString();
				sendResponse(socket, true);
			} else {
				// 失敗: コードがない
				sendResponse(socket, false);
			}
		}

		// レスポンス送信後にソケットを閉じ、コルーチンを再開する
		socket->disconnectFromHost();
		// socketが完全にフラッシュされるのを待たずに閉じて良い（Qtが裏で処理する）が、
		// 念のため少し遅延させてResumeするか、cleanupで行う。

		cleanupAndResume();
	}

	void sendResponse(QTcpSocket *socket, bool success)
	{
		QString content =
			success ? "<html><body><h1>Login Successful</h1><p>You can close this window now.</p></body></html>"
				: "<html><body><h1>Login Failed</h1><p>Invalid request.</p></body></html>";

		QString response = QString("HTTP/1.1 200 OK\r\n"
					   "Content-Type: text/html; charset=utf-8\r\n"
					   "Content-Length: %1\r\n"
					   "Connection: close\r\n"
					   "\r\n"
					   "%2")
					   .arg(content.toUtf8().size())
					   .arg(content);

		socket->write(response.toUtf8());
		socket->flush();
	}

	void cleanupAndResume()
	{
		if (server_) {
			server_->close();
		}
		if (handle_ && !handle_.done()) {
			handle_.resume();
		}
		deleteLater(); // 自分自身を削除
	}

private:
	QTcpServer *server_ = nullptr;
	std::coroutine_handle<> handle_;
	std::string &resultRef_;
};

// コルーチン内で co_await するためのオブジェクト
struct WaitForQtAuthCode {
	uint16_t port;
	std::string result_code;

	bool await_ready() { return false; } // 常にサスペンドする

	void await_suspend(std::coroutine_handle<> h)
	{
		// ワーカーを作成。処理完了後に deleteLater() で自壊する。
		// 親を持たせない(nullptr)ことで、メインスレッドのイベントループで動作させる。
		new AuthCallbackReceiver(port, h, result_code);
	}

	std::string await_resume() { return result_code; }
};

} // namespace

// ユーザーコードプロバイダの実装
Auth::AuthTask<std::string> QtHttpCodeProvider(const std::string & /*authUrl*/)
{
	// ここで Qt のイベントループと協調して停止・待機する
	std::string code = co_await WaitForQtAuthCode{8080};
	co_return code;
}

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

SettingsDialog::~SettingsDialog()
{
	if (googleOAuth2FlowUserAgent_) {
		googleOAuth2FlowUserAgent_->onOpenUrl = nullptr;
	}
}

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

	currentAuthTask_ = runAuthFlow();
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
	Auth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials;
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

// ---------------------------------------------------------
// 2. 認証フローの実体（コルーチン）
// ---------------------------------------------------------
Auth::AuthTask<void> SettingsDialog::runAuthFlow()
{
	// --- [メインスレッド] 準備フェーズ ---

	Auth::GoogleOAuth2ClientCredentials clientCredentials;
	clientCredentials.client_id = clientIdDisplay_->text().toStdString();
	clientCredentials.client_secret = clientSecretDisplay_->text().toStdString();

	// UserAgentの設定
	googleOAuth2FlowUserAgent_ = std::make_shared<Auth::GoogleOAuth2FlowUserAgent>();
	googleOAuth2FlowUserAgent_->onOpenUrl = [this](const std::string &url) {
		QString qUrlStr = QString::fromStdString(url);

		// Qtのスレッドで実行されるように保証
		QMetaObject::invokeMethod(
			this,
			[this, qUrlStr]() {
				bool success = QDesktopServices::openUrl(QUrl(qUrlStr));
				if (!success) {
					QMessageBox msgBox(this);
					msgBox.setIcon(QMessageBox::Warning);
					msgBox.setWindowTitle(tr("Warning"));
					msgBox.setText(tr("Cannot open the authorization URL in the default browser."));
					msgBox.setInformativeText(
						tr("Please manually visit:\n<a href=\"%1\">%1</a>").arg(qUrlStr));
					msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse |
								       Qt::TextBrowserInteraction);
					msgBox.exec();
				}
			},
			Qt::QueuedConnection);
	};

	// Flowの初期化
	googleOAuth2Flow_ = std::make_shared<Auth::GoogleOAuth2Flow>(clientCredentials,
								     "https://www.googleapis.com/auth/youtube.readonly",
								     googleOAuth2FlowUserAgent_, logger_);

	// 結果を格納する変数
	std::optional<Auth::GoogleAuthResponse> result = std::nullopt;
	QString errorMessage;

	// --- [非同期] 実行フェーズ ---
	try {
		// authorizeを呼び出し、完了を待つ
		// Note: 重い処理（トークン交換）の直前に ResumeOnJThread で別スレッドに移動します
		result = co_await googleOAuth2Flow_->authorize("http://127.0.0.1:8080/callback", QtHttpCodeProvider,
							       ResumeOnJThread{currentAuthTaskWorkerThread_});
	} catch (const std::exception &e) {
		errorMessage = QString::fromStdString(e.what());
		logger_->logException(e, "OAuth flow failed");
	}

	// --- [メインスレッド] 結果処理フェーズ ---

	// UI操作のために必ずメインスレッドに戻る
	co_await ResumeOnMainThread{};

	// フローオブジェクトの破棄（必要に応じて）
	googleOAuth2Flow_.reset();
	googleOAuth2FlowUserAgent_.reset();

	// UIのロック解除
	authButton_->setEnabled(true);

	if (result) {
		// 成功時の処理
		logger_->info("Authorization successful!");
		statusLabel_->setText(tr("Authorized (Not Saved)"));

		// 取得したトークンをストアに保存
		// authStore_->setGoogleToken(*result); // ※実装に合わせて調整してください

		QMessageBox::information(this, tr("Success"), tr("Authorization successful!"));

		// 必要ならここでSave処理を呼ぶ
		// storeSettings();

	} else {
		// 失敗時の処理
		statusLabel_->setText(tr("Authorization Failed"));

		QString msg = tr("Authorization failed.");
		if (!errorMessage.isEmpty()) {
			msg += "\n" + errorMessage;
		}
		QMessageBox::critical(this, tr("Error"), msg);
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI

#include "SettingsDialog.moc"
