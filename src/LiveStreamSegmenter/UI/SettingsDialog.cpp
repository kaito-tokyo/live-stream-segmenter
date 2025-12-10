/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
*/
#include <thread> // 【追加】
#include <future>

#include "SettingsDialog.hpp"
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
#include <QTimer> // 【追加】

#include <YouTubeApiClient.hpp>

using namespace KaitoTokyo::LiveStreamSegmenter::API;

namespace KaitoTokyo::LiveStreamSegmenter::UI {

// ==========================================
//  JsonDropArea (変更なし)
// ==========================================

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
	  dropArea_(new JsonDropArea(this)),
	  clientIdDisplay_(new QLineEdit(this)),
	  clientSecretDisplay_(new QLineEdit(this)),
	  loadJsonButton_(new QPushButton(tr("Select File..."), this)),
	  saveButton_(new QPushButton(tr("2. Save Credentials"), this)),
	  authButton_(new QPushButton(tr("3. Authenticate"), this)),
	  statusLabel_(new QLabel(this)),
	  // 新規コンポーネント初期化
	  streamKeyCombo_(new QComboBox(this)),
	  refreshKeysBtn_(new QPushButton(tr("Reload Keys"), this))
{
	setWindowTitle(tr("Settings"));
	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint |
		       Qt::WindowTitleHint);

	setupUi();

	// Existing connections...
	connect(dropArea_, &JsonDropArea::fileDropped, this, &SettingsDialog::onJsonFileSelected);
	connect(dropArea_, &JsonDropArea::clicked, this, &SettingsDialog::onAreaClicked);
	connect(loadJsonButton_, &QPushButton::clicked, this, &SettingsDialog::onLoadJsonClicked);
	connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
	connect(authButton_, &QPushButton::clicked, this, &SettingsDialog::onAuthClicked);

	// Auth connections...
	connect(authManager_, &Auth::GoogleAuthManager::authStateChanged, this, &SettingsDialog::onAuthStateChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginStatusChanged, this,
		&SettingsDialog::onLoginStatusChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginError, this, &SettingsDialog::onLoginError);

	// --- New Stream Key connections ---
	connect(refreshKeysBtn_, &QPushButton::clicked, this, &SettingsDialog::onRefreshKeysClicked);
	// ユーザーが変更した時だけ保存したい
	connect(streamKeyCombo_, QOverload<int>::of(&QComboBox::activated), this,
		&SettingsDialog::onKeySelectionChanged);

	initializeData();
}

void SettingsDialog::setupUi()
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(16);
	mainLayout->setContentsMargins(24, 24, 24, 24);

	// 1. Info
	auto *infoLabel = new QLabel(
		tr("<b>Step 1:</b> Import Credentials.<br><b>Step 2:</b> Authenticate.<br><b>Step 3:</b> Select Stream Key."),
		this);
	infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
	mainLayout->addWidget(infoLabel);

	// 2. Credentials Group
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

	// 3. Auth Group
	auto *authGroup = new QGroupBox(tr("Authorization"), this);
	auto *authLayout = new QVBoxLayout(authGroup);
	authButton_->setEnabled(false);
	statusLabel_->setAlignment(Qt::AlignCenter);
	authLayout->addWidget(authButton_);
	authLayout->addWidget(statusLabel_);
	mainLayout->addWidget(authGroup);

	// --- 4. Stream Key Group (New) ---
	auto *keyGroup = new QGroupBox(tr("Stream Settings"), this);
	auto *keyLayout = new QVBoxLayout(keyGroup);

	auto *keyHeader = new QHBoxLayout();
	keyHeader->addWidget(new QLabel(tr("Target Stream Key:")));
	keyHeader->addStretch();

	refreshKeysBtn_->setCursor(Qt::PointingHandCursor);
	refreshKeysBtn_->setFixedWidth(100);
	refreshKeysBtn_->setEnabled(false); // 認証するまで押せない
	keyHeader->addWidget(refreshKeysBtn_);

	keyLayout->addLayout(keyHeader);

	streamKeyCombo_->setPlaceholderText(tr("Authenticate to fetch keys..."));
	streamKeyCombo_->setEnabled(false);
	keyLayout->addWidget(streamKeyCombo_);

	mainLayout->addWidget(keyGroup);
	mainLayout->addStretch();

	resize(420, 600);
}

// ---------------------------------------------------------
//  Helpers
// ---------------------------------------------------------

QString SettingsDialog::getObsConfigPath(const QString &filename) const
{
	char *path = obs_module_get_config_path(obs_current_module(), filename.toUtf8().constData());
	if (!path)
		return QString();
	QString qPath = QString::fromUtf8(path);
	bfree(path);
	return qPath;
}

bool SettingsDialog::saveCredentialsToStorage(const QString &clientId, const QString &clientSecret)
{
	QString savePath = getObsConfigPath("credentials.json");
	if (savePath.isEmpty())
		return false;
	QFileInfo fileInfo(savePath);
	QDir().mkpath(fileInfo.dir().path());

	QJsonObject root;
	root["client_id"] = clientId;
	root["client_secret"] = clientSecret;

	QFile outFile(savePath);
	if (!outFile.open(QIODevice::WriteOnly))
		return false;
	outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	return true;
}

QJsonObject SettingsDialog::loadCredentialsFromStorage()
{
	QString savePath = getObsConfigPath("credentials.json");
	QFile file(savePath);
	if (!file.open(QIODevice::ReadOnly))
		return QJsonObject();
	return QJsonDocument::fromJson(file.readAll()).object();
}

bool SettingsDialog::parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret)
{
	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
	if (!doc.isObject())
		return false;
	QJsonObject root = doc.object();
	QJsonObject creds;

	if (root.contains("installed"))
		creds = root["installed"].toObject();
	else if (root.contains("web"))
		creds = root["web"].toObject();
	else
		return false;

	outId = creds["client_id"].toString();
	outSecret = creds["client_secret"].toString();
	return (!outId.isEmpty() && !outSecret.isEmpty());
}

// ---------------------------------------------------------
//  Logic
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

		// 【重要】Managerにクレデンシャルを渡す
		authManager_->setCredentials(cid, csecret);
	}

	// 2. AuthStatus更新 (Managerはコンストラクタで自動ロード済み)
	updateAuthUI();
}

void SettingsDialog::onAreaClicked()
{
	onLoadJsonClicked();
}

void SettingsDialog::onLoadJsonClicked()
{
	QString fileName =
		QFileDialog::getOpenFileName(this, tr("Select Credentials JSON"), "", tr("JSON Files (*.json)"));
	if (!fileName.isEmpty())
		onJsonFileSelected(fileName);
}

void SettingsDialog::onJsonFileSelected(const QString &filePath)
{
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
	authButton_->setEnabled(false); // 保存必須

	dropArea_->setText(tr("<b>File Loaded!</b><br>Please click 'Save Credentials'."));
	dropArea_->setStyleSheet(
		"QLabel { border: 2px solid #4CAF50; background-color: #2a3a2a; border-radius: 8px; color: #fff; padding: 20px; }");
}

void SettingsDialog::onSaveClicked()
{
	if (saveCredentialsToStorage(tempClientId_, tempClientSecret_)) {
		// Managerにも反映
		authManager_->setCredentials(tempClientId_, tempClientSecret_);

		updateAuthUI(); // ボタン有効化判定

		QMessageBox::information(this, tr("Saved"), tr("Credentials saved successfully."));
	} else {
		QMessageBox::critical(this, tr("Error"), tr("Failed to save credentials file."));
	}
}

void SettingsDialog::onAuthClicked()
{
	// ログイン処理開始 (Managerに一任)
	authManager_->startLogin();
}

// --- AuthManager Signals ---

void SettingsDialog::onAuthStateChanged()
{
	updateAuthUI();

	if (authManager_->isAuthenticated()) {
		QMessageBox::information(
			this, tr("Success"),
			tr("Authentication successful!\nConnected as: %1").arg(authManager_->currentChannelName()));
	}
}

void SettingsDialog::onLoginStatusChanged(const QString &status)
{
	statusLabel_->setText(status);
	authButton_->setEnabled(false);
}

void SettingsDialog::onLoginError(const QString &message)
{
	updateAuthUI(); // ボタン復帰
	QMessageBox::critical(this, tr("Login Error"), message);
}

void SettingsDialog::onLinkDocClicked()
{
	QDesktopServices::openUrl(QUrl("https://console.cloud.google.com/apis/credentials"));
}

// ---------------------------------------------------------
//  Stream Key Logic
// ---------------------------------------------------------

void SettingsDialog::onRefreshKeysClicked()
{
	refreshKeysBtn_->setEnabled(false);
	refreshKeysBtn_->setText(tr("Loading..."));
	streamKeyCombo_->clear();
	streamKeyCombo_->addItem(tr("Loading..."));

	YouTubeApiClient client([this]() {
		auto promise = std::make_shared<std::promise<std::string>>();
		authManager_->getAccessToken(
			[promise](const QString &token) { promise->set_value(token.toStdString()); },
			[promise](const QString &) { promise->set_value(""); });
		return promise->get_future();
	});

	auto streamKeys = client.listStreamKeys();

	QTimer::singleShot(0, this, [this, streamKeys]() { updateStreamKeyList(streamKeys); });
}

void SettingsDialog::updateStreamKeyList(const std::vector<API::YouTubeStreamKey> &keys)
{
	refreshKeysBtn_->setEnabled(true);
	refreshKeysBtn_->setText(tr("Reload Keys"));
	streamKeyCombo_->clear();

	if (keys.empty()) {
		streamKeyCombo_->setPlaceholderText(tr("No RTMP keys found."));
		return;
	}

	// 保存済みのIDを確認して、選択状態を復元する
	QString savedId = loadSavedStreamKeyId();
	int selectIndex = -1;

	for (const auto &key : keys) {
		// Pure C++ Struct -> Qt String
		QString title = QString::fromStdString(key.snippet_title);
		QString id = QString::fromStdString(key.id);
		QString streamName = QString::fromStdString(key.cdn_ingestionInfo_streamName);
		QString desc = QString::fromStdString(key.cdn_resolution);

		QString displayText = QString("%1 (%2)").arg(title, desc);

		// ItemDataに ID と StreamName をパイプ区切りで格納
		// (保存時に両方必要なため)
		QString itemData = QString("%1|%2").arg(id, streamName);

		streamKeyCombo_->addItem(displayText, itemData);

		if (id == savedId) {
			selectIndex = streamKeyCombo_->count() - 1;
		}
	}

	if (selectIndex >= 0) {
		streamKeyCombo_->setCurrentIndex(selectIndex);
	} else {
		// 保存されたものが見つからない場合、未選択状態にするか先頭を選ぶ
		// ここではユーザーに選ばせるため、index 0にはしないでおくことも可能だが
		// 親切のために0を選んでおく
		streamKeyCombo_->setCurrentIndex(0);
		// 自動で保存はしない（ユーザーの明示的な選択を待つ）
	}
}

void SettingsDialog::onKeySelectionChanged(int index)
{
	if (index < 0)
		return;

	QString data = streamKeyCombo_->itemData(index).toString();
	QStringList parts = data.split("|");
	if (parts.size() < 2)
		return;

	QString id = parts[0];
	QString streamName = parts[1];
	QString title = streamKeyCombo_->itemText(index);

	// 選択された瞬間に保存する
	saveStreamKeySetting(id, streamName, title);
}

// ---------------------------------------------------------
//  Config Persistence (config.json)
// ---------------------------------------------------------

void SettingsDialog::saveStreamKeySetting(const QString &id, const QString &streamName, const QString &title)
{
	QString savePath = getObsConfigPath("config.json");
	if (savePath.isEmpty())
		return;

	// 既存の設定を読み込んでマージするのが理想だが、
	// 今回はストリームキー設定が主なので上書きでも可。
	// 安全のため、もしファイルがあれば読み込んで追記する形にする。
	QJsonObject root;
	QFile file(savePath);
	if (file.open(QIODevice::ReadOnly)) {
		root = QJsonDocument::fromJson(file.readAll()).object();
		file.close();
	}

	root["stream_id"] = id;
	root["stream_key"] = streamName;
	root["stream_title"] = title; // UI復元用のキャッシュ（無くても良い）
	root["updated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
		file.close();
	}
}

QString SettingsDialog::loadSavedStreamKeyId()
{
	QString savePath = getObsConfigPath("config.json");
	QFile file(savePath);
	if (file.open(QIODevice::ReadOnly)) {
		QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
		return root["stream_id"].toString();
	}
	return QString();
}

// ---------------------------------------------------------
//  Auth / Update Logic
// ---------------------------------------------------------

void SettingsDialog::updateAuthUI()
{
	// ... (既存ロジック)

	bool isAuth = authManager_->isAuthenticated();
	if (isAuth) {
		statusLabel_->setText(QString("✅ Connected: %1").arg(authManager_->currentChannelName()));
		statusLabel_->setStyleSheet(
			"color: #4CAF50; padding: 4px; border: 1px solid #4CAF50; border-radius: 4px;");
		authButton_->setText(tr("Re-Authenticate"));
		authButton_->setEnabled(true);

		// キー取得ボタンを有効化
		refreshKeysBtn_->setEnabled(true);
		streamKeyCombo_->setEnabled(true);

		// リストが空ならプレフィックスを表示
		if (streamKeyCombo_->count() == 0) {
			streamKeyCombo_->setPlaceholderText(tr("Click 'Reload Keys' to fetch list"));

			// 保存済み設定があれば、とりあえず表示だけでもしておく（API叩かなくても）
			QString savedId = loadSavedStreamKeyId();
			if (!savedId.isEmpty()) {
				// 保存ファイルから名前も読み出せればベストだが、ここではReloadを促す
				streamKeyCombo_->setPlaceholderText(
					tr("Saved ID: %1 (Click Reload to verify)").arg(savedId));
			}
		}

	} else {
		statusLabel_->setText(tr("⚠️ Not Connected"));
		statusLabel_->setStyleSheet(
			"color: #F44747; padding: 4px; border: 1px solid #F44747; border-radius: 4px;");
		authButton_->setText(tr("3. Authenticate"));
		authButton_->setEnabled(!tempClientId_.isEmpty());

		// 未認証ならキー操作無効
		refreshKeysBtn_->setEnabled(false);
		streamKeyCombo_->setEnabled(false);
		streamKeyCombo_->setPlaceholderText(tr("Authenticate first"));
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
