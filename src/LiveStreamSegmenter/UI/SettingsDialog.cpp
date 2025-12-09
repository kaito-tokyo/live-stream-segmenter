/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
*/

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
	  authManager_(new Auth::GoogleAuthManager(this)), // Manager生成
	  dropArea_(new JsonDropArea(this)),
	  clientIdDisplay_(new QLineEdit(this)),
	  clientSecretDisplay_(new QLineEdit(this)),
	  loadJsonButton_(new QPushButton(tr("Select File..."), this)),
	  saveButton_(new QPushButton(tr("2. Save Credentials"), this)),
	  authButton_(new QPushButton(tr("3. Authenticate"), this)),
	  statusLabel_(new QLabel(this))
{
	setWindowTitle(tr("Settings"));
	setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint |
		       Qt::WindowTitleHint);

	setupUi();

	// UI Connections
	connect(dropArea_, &JsonDropArea::fileDropped, this, &SettingsDialog::onJsonFileSelected);
	connect(dropArea_, &JsonDropArea::clicked, this, &SettingsDialog::onAreaClicked);
	connect(loadJsonButton_, &QPushButton::clicked, this, &SettingsDialog::onLoadJsonClicked);
	connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
	connect(authButton_, &QPushButton::clicked, this, &SettingsDialog::onAuthClicked);

	// AuthManager Connections
	connect(authManager_, &Auth::GoogleAuthManager::authStateChanged, this, &SettingsDialog::onAuthStateChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginStatusChanged, this,
		&SettingsDialog::onLoginStatusChanged);
	connect(authManager_, &Auth::GoogleAuthManager::loginError, this, &SettingsDialog::onLoginError);

	initializeData();
}

void SettingsDialog::setupUi()
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(16);
	mainLayout->setContentsMargins(24, 24, 24, 24);

	// 説明
	auto *infoLabel = new QLabel(tr("<b>Step 1:</b> Drop your GCP Credentials JSON file here.<br>"
					"<b>Step 2:</b> Review and Save.<br>"
					"<b>Step 3:</b> Authenticate with YouTube."),
				     this);
	infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px; margin-bottom: 8px;");
	mainLayout->addWidget(infoLabel);

	// Credentials Group
	auto *credGroup = new QGroupBox(tr("Credentials Setup"), this);
	auto *formLayout = new QVBoxLayout(credGroup);
	formLayout->setSpacing(12);
	formLayout->addWidget(dropArea_);

	auto *detailsLayout = new QFormLayout();
	detailsLayout->setLabelAlignment(Qt::AlignLeft);

	clientIdDisplay_->setReadOnly(true);
	clientIdDisplay_->setPlaceholderText("(Waiting for file...)");
	clientIdDisplay_->setStyleSheet("color: #cccccc; background: #222; border: 1px solid #444;");
	detailsLayout->addRow(tr("Client ID:"), clientIdDisplay_);

	clientSecretDisplay_->setReadOnly(true);
	clientSecretDisplay_->setEchoMode(QLineEdit::Password);
	clientSecretDisplay_->setPlaceholderText("(Hidden)");
	clientSecretDisplay_->setStyleSheet("color: #cccccc; background: #222; border: 1px solid #444;");
	detailsLayout->addRow(tr("Client Secret:"), clientSecretDisplay_);

	formLayout->addLayout(detailsLayout);

	saveButton_->setCursor(Qt::PointingHandCursor);
	saveButton_->setEnabled(false);
	saveButton_->setStyleSheet("font-weight: bold; margin-top: 4px; height: 30px;");
	formLayout->addWidget(saveButton_);

	mainLayout->addWidget(credGroup);

	// Auth Group
	auto *authGroup = new QGroupBox(tr("Authorization"), this);
	auto *authLayout = new QVBoxLayout(authGroup);

	authButton_->setCursor(Qt::PointingHandCursor);
	authButton_->setStyleSheet("padding: 8px; font-weight: bold;");
	authButton_->setEnabled(false);

	statusLabel_->setAlignment(Qt::AlignCenter);

	authLayout->addWidget(authButton_);
	authLayout->addWidget(statusLabel_);

	mainLayout->addWidget(authGroup);
	mainLayout->addStretch();

	resize(420, 520);
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

void SettingsDialog::updateAuthUI()
{
	bool hasCreds = !tempClientId_.isEmpty();
	bool isAuth = authManager_->isAuthenticated();

	if (isAuth) {
		statusLabel_->setText(QString("✅ Connected: %1").arg(authManager_->currentChannelName()));
		statusLabel_->setStyleSheet(
			"color: #4CAF50; padding: 4px; border: 1px solid #4CAF50; border-radius: 4px;");
		authButton_->setText(tr("Re-Authenticate"));
		authButton_->setEnabled(true);
	} else {
		statusLabel_->setText(tr("⚠️ Not Connected"));
		statusLabel_->setStyleSheet(
			"color: #F44747; padding: 4px; border: 1px solid #F44747; border-radius: 4px;");
		authButton_->setText(tr("3. Authenticate"));
		authButton_->setEnabled(hasCreds); // クレデンシャルがあれば押せる
	}
}

void SettingsDialog::onLinkDocClicked()
{
	QDesktopServices::openUrl(QUrl("https://console.cloud.google.com/apis/credentials"));
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
