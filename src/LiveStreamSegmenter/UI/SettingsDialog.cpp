/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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
// GoogleTokenStateへの変換用
#include "../Auth/GoogleTokenState.hpp"

using namespace KaitoTokyo::LiveStreamSegmenter::API;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

// ==========================================
//  JsonDropArea (変更なし)
// ==========================================
// ... (JsonDropAreaの実装はそのまま) ...
JsonDropArea::JsonDropArea(QWidget *parent) : QLabel(parent)
{
	setText(tr("<b>Drag & Drop</b> credentials.json here<br>"
		   "<span style='font-size:10px; color:#888;'>(or click to browse)</span>"));
	setAlignment(Qt::AlignCenter);
	setAcceptDrops(true);
	setWordWrap(true);
	setStyleSheet("QLabel {"
		      "  border: 2px dashed #666;"
		      "  border-radius: 8px;"
		      "  background-color: #2a2a2a;"
		      "  color: #ccc;"
		      "  padding: 20px;"
		      "}"
		      "QLabel:hover {"
		      "  border-color: #4EC9B0;"
		      "  background-color: #333;"
		      "}");
	setCursor(Qt::PointingHandCursor);
}

void JsonDropArea::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
		setStyleSheet(
			"QLabel { border: 2px dashed #4CAF50; background-color: #333; border-radius: 8px; color: #fff; padding: 20px; }");
	}
}

void JsonDropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
	Q_UNUSED(event);
	setStyleSheet(
		"QLabel { border: 2px dashed #666; background-color: #2a2a2a; border-radius: 8px; color: #ccc; padding: 20px; } QLabel:hover { border-color: #4EC9B0; background-color: #333; }");
}

void JsonDropArea::dropEvent(QDropEvent *event)
{
	setStyleSheet(
		"QLabel { border: 2px dashed #666; background-color: #2a2a2a; border-radius: 8px; color: #ccc; padding: 20px; } QLabel:hover { border-color: #4EC9B0; background-color: #333; }");
	const QList<QUrl> urls = event->mimeData()->urls();
	if (!urls.isEmpty()) {
		emit fileDropped(urls.first().toLocalFile());
	}
}

void JsonDropArea::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	emit clicked();
}

// ==========================================
//  SettingsDialog
// ==========================================

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent),
	  authManager_(new Auth::GoogleAuthManager(this)),
	  authPollTimer_(new QTimer(this)), // Timer初期化
	  dropArea_(new JsonDropArea(this)),
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

	// Connections
	connect(dropArea_, &JsonDropArea::fileDropped, this, &SettingsDialog::onJsonFileSelected);
	connect(dropArea_, &JsonDropArea::clicked, this, &SettingsDialog::onAreaClicked);
	connect(loadJsonButton_, &QPushButton::clicked, this, &SettingsDialog::onLoadJsonClicked);
	connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
	
	// Auth Trigger
	connect(authButton_, &QPushButton::clicked, this, &SettingsDialog::onAuthClicked);
	
	// Polling Timer
	connect(authPollTimer_, &QTimer::timeout, this, &SettingsDialog::onAuthPollTimer);

	// Auth Manager Signals
	connect(authManager_, &Auth::GoogleAuthManager::authStateChanged, this, &SettingsDialog::onAuthStateChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginStatusChanged, this,
		&SettingsDialog::onLoginStatusChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginError, this, &SettingsDialog::onLoginError);

	// Stream Key
	connect(refreshKeysBtn_, &QPushButton::clicked, this, &SettingsDialog::onRefreshKeysClicked);
	connect(streamKeyCombo_, QOverload<int>::of(&QComboBox::activated), this,
		&SettingsDialog::onKeySelectionChanged);
	connect(new QPushButton(this), &QPushButton::clicked, this, &SettingsDialog::onLinkDocClicked); // ダミー接続(UI定義側で接続してるので実際は不要だが念のため)

	initializeData();
}

SettingsDialog::~SettingsDialog()
{
	// フロー実行中なら停止
	if (oauthFlow_) {
		oauthFlow_->stop();
	}
}

// ... (setupUi, Helpersの実装は変更なしのため省略。initializeData等は変更なし) ...
void SettingsDialog::setupUi()
{
	// (元の実装と同じ)
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(16);
	mainLayout->setContentsMargins(24, 24, 24, 24);

	auto *infoLabel = new QLabel(
		tr("<b>Step 1:</b> Import Credentials.<br><b>Step 2:</b> Authenticate.<br><b>Step 3:</b> Select Stream Key."),
		this);
	infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
	mainLayout->addWidget(infoLabel);

	auto *credGroup = new QGroupBox(tr("Credentials"), this);
	auto *credLayout = new QVBoxLayout(credGroup);
	credLayout->addWidget(dropArea_);

	auto *detailsLayout = new QFormLayout();
	clientIdDisplay_->setReadOnly(true);
	clientIdDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout->addRow(tr("Client ID:"), clientIdDisplay_);

	clientSecretDisplay_->setReadOnly(true);
	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setStyleSheet("background: #222; border: 1px solid #444; color: #ccc;");
	detailsLayout->addRow(tr("Client Secret:"), clientSecretDisplay_);
	credLayout->addLayout(detailsLayout);

	saveButton_->setEnabled(false);
	credLayout->addWidget(saveButton_);
	mainLayout->addWidget(credGroup);

	auto *authGroup = new QGroupBox(tr("Authorization"), this);
	auto *authLayout = new QVBoxLayout(authGroup);
	authButton_->setEnabled(false);
	statusLabel_->setAlignment(Qt::AlignCenter);
	authLayout->addWidget(authButton_);
	authLayout->addWidget(statusLabel_);
	mainLayout->addWidget(authGroup);

	auto *keyGroup = new QGroupBox(tr("Stream Settings"), this);
	auto *keyLayout = new QVBoxLayout(keyGroup);

	auto *keyHeader = new QHBoxLayout();
	keyHeader->addWidget(new QLabel(tr("Target Stream Key:")));
	keyHeader->addStretch();

	refreshKeysBtn_->setCursor(Qt::PointingHandCursor);
	refreshKeysBtn_->setFixedWidth(100);
	refreshKeysBtn_->setEnabled(false); 
	keyHeader->addWidget(refreshKeysBtn_);

	keyLayout->addLayout(keyHeader);

	streamKeyCombo_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyCombo_->setEnabled(false);
	keyLayout->addWidget(streamKeyCombo_);

	mainLayout->addWidget(keyGroup);
	mainLayout->addStretch();

	resize(420, 600);
}

// ... (Helpers: getObsConfigPath, saveCredentialsToStorage, loadCredentialsFromStorage, parseCredentialJson, saveStreamKeySetting, loadSavedStreamKeyId は変更なし) ...
QString SettingsDialog::getObsConfigPath(const QString &filename) const {
    char *path = obs_module_get_config_path(obs_current_module(), filename.toUtf8().constData());
    if (!path) return QString();
    QString qPath = QString::fromUtf8(path);
    bfree(path);
    return qPath;
}

bool SettingsDialog::saveCredentialsToStorage(const QString &clientId, const QString &clientSecret) {
    // ... (元の実装と同じ)
    QString savePath = getObsConfigPath("credentials.json");
    if (savePath.isEmpty()) return false;
    QFileInfo fileInfo(savePath);
    QDir().mkpath(fileInfo.dir().path());
    QJsonObject root;
    root["client_id"] = clientId;
    root["client_secret"] = clientSecret;
    QFile outFile(savePath);
    if (!outFile.open(QIODevice::WriteOnly)) return false;
    outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject SettingsDialog::loadCredentialsFromStorage() {
    // ... (元の実装と同じ)
    QString savePath = getObsConfigPath("credentials.json");
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool SettingsDialog::parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret) {
    // ... (元の実装と同じ)
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return false;
    QJsonObject root = doc.object();
    QJsonObject creds;
    if (root.contains("installed")) creds = root["installed"].toObject();
    else if (root.contains("web")) creds = root["web"].toObject();
    else return false;
    outId = creds["client_id"].toString();
    outSecret = creds["client_secret"].toString();
    return (!outId.isEmpty() && !outSecret.isEmpty());
}

// ---------------------------------------------------------
//  Logic (Updated for GoogleOAuth2Flow)
// ---------------------------------------------------------

void SettingsDialog::initializeData()
{
	// 1. Credentials読み込み
	QJsonObject creds = loadCredentialsFromStorage();
	QString cid = creds["client_id"].toString();
	QString csecret = creds["client_secret"].toString();

	if (!cid.isEmpty() && !csecret.isEmpty()) {
		tempClientId_ = cid;
		tempClientSecret_ = csecret;
		clientIdDisplay_->setText(cid);
		clientSecretDisplay_->setText(csecret);

		saveButton_->setEnabled(true);
		dropArea_->setText(tr("<b>Credentials Loaded.</b><br>Drag & Drop to update."));

		authManager_->setCredentials(cid, csecret);
	}

	updateAuthUI();
}

void SettingsDialog::onAreaClicked() { onLoadJsonClicked(); }
void SettingsDialog::onLoadJsonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Credentials JSON"), "", tr("JSON Files (*.json)"));
    if (!fileName.isEmpty()) onJsonFileSelected(fileName);
}
void SettingsDialog::onJsonFileSelected(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file."));
        return;
    }
    QString cid, csecret;
    if (!parseCredentialJson(file.readAll(), cid, csecret)) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid JSON format."));
        return;
    }
    tempClientId_ = cid;
    tempClientSecret_ = csecret;
    clientIdDisplay_->setText(cid);
    clientSecretDisplay_->setText(csecret);
    saveButton_->setEnabled(true);
    authButton_->setEnabled(false);
    dropArea_->setText(tr("<b>File Loaded!</b><br>Please click 'Save Credentials'."));
    dropArea_->setStyleSheet("QLabel { border: 2px solid #4CAF50; background-color: #2a3a2a; border-radius: 8px; color: #fff; padding: 20px; }");
}
void SettingsDialog::onSaveClicked() {
    if (saveCredentialsToStorage(tempClientId_, tempClientSecret_)) {
        authManager_->setCredentials(tempClientId_, tempClientSecret_);
        updateAuthUI();
        QMessageBox::information(this, tr("Saved"), tr("Credentials saved successfully."));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save credentials file."));
    }
}

// ---------------------------------------------------------
//  New Auth Logic
// ---------------------------------------------------------

void SettingsDialog::onAuthClicked()
{
	// UIロック
	authButton_->setEnabled(false);
	authButton_->setText(tr("Waiting for Browser..."));
	statusLabel_->setText(tr("Auth flow started..."));

	// 1. UserAgent (UI callbacks) の設定
	Auth::GoogleOAuth2FlowUserAgent userAgent;

	// ブラウザ起動
	userAgent.openBrowser = [](const std::string &url) {
		QDesktopServices::openUrl(QUrl(QString::fromStdString(url)));
	};

	// 成功画面
	userAgent.onLoginSuccess = [](const auto &, auto &res) {
		res.set_content(
			"<html><head><title>Success</title><style>"
			"body { background: #f0f2f5; font-family: sans-serif; text-align: center; padding-top: 50px; }"
			".box { background: white; padding: 30px; border-radius: 8px; display: inline-block; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
			"h1 { color: #4CAF50; }"
			"</style></head><body>"
			"<div class='box'>"
			"<h1>Authentication Successful!</h1>"
			"<p>You can verify the result in OBS settings dialog.</p>"
			"<p>This window will close automatically.</p>"
			"</div>"
			"<script>setTimeout(function(){ window.close(); }, 3000);</script>"
			"</body></html>",
			"text/html");
	};

	// 失敗画面
	userAgent.onLoginFailure = [](const auto &, auto &res) {
		res.status = 400;
		res.set_content(
			"<html><head><title>Failed</title></head><body style='text-align:center; padding-top:50px; font-family:sans-serif;'>"
			"<h1 style='color:red;'>Authentication Failed</h1>"
			"<p>Please check the logs in OBS.</p>"
			"</body></html>",
			"text/html");
	};

	// 2. Flow作成
	// ロガーは本来DIすべきだが、ここでは簡易的にnullptr (またはGlobal Logger Wrapper)
	// プロジェクト構造に合わせてILoggerの実装を渡すのがベスト
	auto logger = std::make_shared<Logger::ILogger>(); // ※ ILoggerが抽象クラスなら実装クラスが必要

	// 必要権限
	std::string scopes = "https://www.googleapis.com/auth/youtube.readonly";

	oauthFlow_ = std::make_unique<Auth::GoogleOAuth2Flow>(
		tempClientId_.toStdString(),
		tempClientSecret_.toStdString(),
		scopes,
		userAgent,
		logger // ロガーを渡す
	);

	// 3. 開始 (非同期)
	pendingAuthFuture_ = oauthFlow_->start();

	// 4. 完了監視用ポーリング開始 (100msごとに確認)
	authPollTimer_->start(100);
}

void SettingsDialog::onAuthPollTimer()
{
	// Futureが完了したかチェック
	// wait_for(0) で即時チェック
	if (pendingAuthFuture_.valid() &&
	    pendingAuthFuture_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		
		// 完了したのでタイマー停止
		authPollTimer_->stop();

		try {
			// 結果取得
			auto result = pendingAuthFuture_.get();

			if (result.has_value()) {
				// 成功: Managerにトークンを渡す
				Auth::GoogleTokenResponse resp = result.value();
				
				// TokenResponse -> TokenState 変換
				Auth::GoogleTokenState state;
				state.updateFromTokenResponse(resp);
				// Email等の取得はManager側でやるか、Flow側でやるかだが、
				// Flowはシンプル化されたので、必要ならManagerが後でUserInfoを叩く
				
				authManager_->updateTokenState(state);
				
				QMessageBox::information(this, tr("Auth"), tr("Authentication succeeded!"));
			} else {
				// 失敗/キャンセル
				QMessageBox::warning(this, tr("Auth"), tr("Authentication was cancelled or failed."));
			}
		} catch (const std::exception &e) {
			QMessageBox::critical(this, tr("Auth Error"), QString("Error: %1").arg(e.what()));
		}

		// UI復帰
		updateAuthUI();
		
		// Flow破棄 (再実行に備える)
		oauthFlow_.reset();
	}
}

// ... (残りのスロット実装は変更なし) ...

void SettingsDialog::onAuthStateChanged() { updateAuthUI(); }
void SettingsDialog::onLoginStatusChanged(const QString &status) { statusLabel_->setText(status); }
void SettingsDialog::onLoginError(const QString &message) { updateAuthUI(); QMessageBox::critical(this, tr("Error"), message); }
void SettingsDialog::onLinkDocClicked() { QDesktopServices::openUrl(QUrl("https://console.cloud.google.com/apis/credentials")); }

// Stream Key Logic
void SettingsDialog::onRefreshKeysClicked() {
    refreshKeysBtn_->setEnabled(false);
    refreshKeysBtn_->setText(tr("Loading..."));
    streamKeyCombo_->clear();
    streamKeyCombo_->addItem(tr("Loading..."));

    // 新しいApiClientを使って取得 (同期/非同期はClientの実装次第だが、ここでは簡易的に)
    YouTubeApiClient client([this]() {
        auto promise = std::make_shared<std::promise<std::string>>();
        // Managerから現在の有効なトークンをもらう
        QString token = authManager_->getAccessToken(); 
        promise->set_value(token.toStdString());
        return promise->get_future();
    });

    // GUIスレッドでブロックしないように別スレッドで実行推奨だが、今回はそのまま
    std::thread([this, client]() mutable {
        try {
            auto keys = client.listStreamKeys();
            // UI更新はメインスレッドで
            QTimer::singleShot(0, this, [this, keys]() {
                updateStreamKeyList(keys);
            });
        } catch(...) {
             QTimer::singleShot(0, this, [this]() {
                refreshKeysBtn_->setEnabled(true);
                refreshKeysBtn_->setText(tr("Reload Keys"));
                streamKeyCombo_->clear();
                streamKeyCombo_->setPlaceholderText(tr("Failed to load keys."));
            });
        }
    }).detach();
}

void SettingsDialog::updateStreamKeyList(const std::vector<API::YouTubeStreamKey> &keys) {
    // ... (元の実装と同じ)
    refreshKeysBtn_->setEnabled(true);
    refreshKeysBtn_->setText(tr("Reload Keys"));
    streamKeyCombo_->clear();
    if (keys.empty()) {
        streamKeyCombo_->setPlaceholderText(tr("No RTMP keys found."));
        return;
    }
    QString savedId = loadSavedStreamKeyId();
    int selectIndex = -1;
    for (const auto &key : keys) {
        QString title = QString::fromStdString(key.snippet_title);
        QString id = QString::fromStdString(key.id);
        QString streamName = QString::fromStdString(key.cdn_ingestionInfo_streamName);
        QString desc = QString::fromStdString(key.cdn_resolution);
        QString displayText = QString("%1 (%2)").arg(title, desc);
        QString itemData = QString("%1|%2").arg(id, streamName);
        streamKeyCombo_->addItem(displayText, itemData);
        if (id == savedId) selectIndex = streamKeyCombo_->count() - 1;
    }
    if (selectIndex >= 0) streamKeyCombo_->setCurrentIndex(selectIndex);
    else streamKeyCombo_->setCurrentIndex(0);
}

void SettingsDialog::onKeySelectionChanged(int index) {
    if (index < 0) return;
    QString data = streamKeyCombo_->itemData(index).toString();
    QStringList parts = data.split("|");
    if (parts.size() < 2) return;
    saveStreamKeySetting(parts[0], parts[1], streamKeyCombo_->itemText(index));
}

void SettingsDialog::saveStreamKeySetting(const QString &id, const QString &streamName, const QString &title) {
    // ... (元の実装と同じ)
    QString savePath = getObsConfigPath("config.json");
    if (savePath.isEmpty()) return;
    QJsonObject root;
    QFile file(savePath);
    if (file.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    }
    root["stream_id"] = id;
    root["stream_key"] = streamName;
    root["stream_title"] = title;
    root["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

QString SettingsDialog::loadSavedStreamKeyId() {
    QString savePath = getObsConfigPath("config.json");
    QFile file(savePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        return root["stream_id"].toString();
    }
    return QString();
}

void SettingsDialog::updateAuthUI()
{
	bool isAuth = authManager_->isAuthenticated();
	if (isAuth) {
		statusLabel_->setText(QString("✅ Connected"));
		statusLabel_->setStyleSheet("color: #4CAF50; padding: 4px; border: 1px solid #4CAF50; border-radius: 4px;");
		authButton_->setText(tr("Re-Authenticate"));
		authButton_->setEnabled(true);
		refreshKeysBtn_->setEnabled(true);
		streamKeyCombo_->setEnabled(true);
        if (streamKeyCombo_->count() == 0) streamKeyCombo_->setPlaceholderText(tr("Click 'Reload Keys' to fetch list"));
	} else {
		statusLabel_->setText(tr("⚠️ Not Connected"));
		statusLabel_->setStyleSheet("color: #F44747; padding: 4px; border: 1px solid #F44747; border-radius: 4px;");
		authButton_->setText(tr("3. Authenticate"));
		authButton_->setEnabled(!tempClientId_.isEmpty());
		refreshKeysBtn_->setEnabled(false);
		streamKeyCombo_->setEnabled(false);
		streamKeyCombo_->setPlaceholderText(tr("Authenticate first"));
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
