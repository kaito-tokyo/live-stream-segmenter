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
#include <obs-module.h> // OBS API

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

// ==========================================
//  JsonDropArea Implementation
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
	if (urls.isEmpty())
		return;
	QString filePath = urls.first().toLocalFile();
	if (!filePath.isEmpty()) {
		emit fileDropped(filePath);
	}
}

void JsonDropArea::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	emit clicked();
}

// ==========================================
//  SettingsDialog Implementation
// ==========================================

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent),
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

	// UIシグナル接続
	connect(dropArea_, &JsonDropArea::fileDropped, this, &SettingsDialog::onJsonFileSelected);
	connect(dropArea_, &JsonDropArea::clicked, this, &SettingsDialog::onAreaClicked);
	connect(loadJsonButton_, &QPushButton::clicked, this,
		&SettingsDialog::onLoadJsonClicked); // ボタンでも開けるように
	connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
	connect(authButton_, &QPushButton::clicked, this, &SettingsDialog::onAuthClicked);

	initializeData();
}

void SettingsDialog::setupUi()
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(16);
	mainLayout->setContentsMargins(24, 24, 24, 24);

	// 説明
	auto *infoLabel = new QLabel(tr("<b>Step 1:</b> Drop your GCP Credentials JSON file here.<br>"
					"<b>Step 2:</b> Review and Save (The file is copied internally).<br>"
					"<b>Step 3:</b> Authenticate with YouTube."),
				     this);
	infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px; margin-bottom: 8px;");
	mainLayout->addWidget(infoLabel);

	// 認証情報グループ
	auto *credGroup = new QGroupBox(tr("Credentials Setup"), this);
	auto *formLayout = new QVBoxLayout(credGroup);
	formLayout->setSpacing(12);

	// Drop Area
	formLayout->addWidget(dropArea_);

	// 詳細表示
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

	// Save Button
	saveButton_->setCursor(Qt::PointingHandCursor);
	saveButton_->setEnabled(false);
	saveButton_->setStyleSheet("font-weight: bold; margin-top: 4px; height: 30px;");
	formLayout->addWidget(saveButton_);

	mainLayout->addWidget(credGroup);

	// 認証グループ
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

	resize(420, 500);
}

// ---------------------------------------------------------
//  OBS API / Logic Helpers (ここにロジックを集約)
// ---------------------------------------------------------

QString SettingsDialog::getObsConfigPath(const QString &filename) const
{
	// OBS API呼び出しはここだけ
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

	// ディレクトリ作成
	QFileInfo fileInfo(savePath);
	QDir().mkpath(fileInfo.dir().path());

	// JSON作成
	QJsonObject root;
	root["client_id"] = clientId;
	root["client_secret"] = clientSecret;

	// 書き込み
	QFile outFile(savePath);
	if (!outFile.open(QIODevice::WriteOnly))
		return false;

	outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	outFile.close();

	return true;
}

QJsonObject SettingsDialog::loadCredentialsFromStorage()
{
	QString savePath = getObsConfigPath("credentials.json");
	QFile file(savePath);
	if (!file.open(QIODevice::ReadOnly))
		return QJsonObject();

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	return doc.object();
}

bool SettingsDialog::parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret)
{
	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
	if (!doc.isObject())
		return false;

	QJsonObject root = doc.object();
	QJsonObject creds;

	if (root.contains("installed")) {
		creds = root["installed"].toObject();
	} else if (root.contains("web")) {
		creds = root["web"].toObject();
	} else {
		return false;
	}

	outId = creds["client_id"].toString();
	outSecret = creds["client_secret"].toString();

	return (!outId.isEmpty() && !outSecret.isEmpty());
}

// ---------------------------------------------------------
//  UI Slots (司令塔: ロジックを呼び出しUIを更新する)
// ---------------------------------------------------------

void SettingsDialog::initializeData()
{
	// 起動時に保存済みデータをロード
	QJsonObject savedData = loadCredentialsFromStorage();

	QString cid = savedData["client_id"].toString();
	QString csecret = savedData["client_secret"].toString();

	if (!cid.isEmpty() && !csecret.isEmpty()) {
		// メモリ反映
		tempClientId_ = cid;
		tempClientSecret_ = csecret;

		// UI反映
		clientIdDisplay_->setText(cid);
		clientSecretDisplay_->setText(csecret);

		// 状態更新
		authButton_->setEnabled(true);
		saveButton_->setEnabled(true);

		dropArea_->setText(tr("<b>Loaded from storage.</b><br>Drag & Drop to update."));
		statusLabel_->setText(tr("Credentials Ready (Not Authenticated)"));
	} else {
		updateAuthStatus(false);
	}
}

void SettingsDialog::onAreaClicked()
{
	onLoadJsonClicked();
}

void SettingsDialog::onLoadJsonClicked()
{
	QString fileName =
		QFileDialog::getOpenFileName(this, tr("Select Credentials JSON"), "", tr("JSON Files (*.json)"));

	if (!fileName.isEmpty()) {
		onJsonFileSelected(fileName);
	}
}

void SettingsDialog::onJsonFileSelected(const QString &filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Error"), tr("Could not open file."));
		return;
	}

	// ロジック呼び出し
	QString cid, csecret;
	bool success = parseCredentialJson(file.readAll(), cid, csecret);

	if (!success) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Invalid JSON format.\nMake sure you downloaded the 'OAuth 2.0 Client ID' JSON (Desktop App)."));
		return;
	}

	// メモリ反映
	tempClientId_ = cid;
	tempClientSecret_ = csecret;

	// UI反映
	clientIdDisplay_->setText(cid);
	clientSecretDisplay_->setText(csecret);

	saveButton_->setEnabled(true);
	authButton_->setEnabled(false); // 保存を強制

	dropArea_->setText(tr("<b>File Loaded!</b><br>Please click 'Save Credentials'."));
	dropArea_->setStyleSheet(
		"QLabel { border: 2px solid #4CAF50; background-color: #2a3a2a; border-radius: 8px; color: #fff; padding: 20px; }");
}

void SettingsDialog::onSaveClicked()
{
	// ロジック呼び出し
	bool success = saveCredentialsToStorage(tempClientId_, tempClientSecret_);

	if (success) {
		authButton_->setEnabled(true);
		QMessageBox::information(this, tr("Saved"),
					 tr("Credentials saved successfully.\n"
					    "You can now safely delete the source JSON file."));
	} else {
		QMessageBox::critical(this, tr("Error"), tr("Failed to save credentials file."));
	}
}

void SettingsDialog::onAuthClicked()
{
	// 認証ロジック (次回実装)
	QMessageBox::information(this, tr("Auth"), tr("Starting OAuth flow..."));
}

void SettingsDialog::updateAuthStatus(bool isConnected, const QString &accountName)
{
	if (isConnected) {
		statusLabel_->setText(QString("✅ Connected: %1").arg(accountName));
		statusLabel_->setStyleSheet("color: #4CAF50;");
	} else {
		statusLabel_->setText("⚠️ Not Connected");
		statusLabel_->setStyleSheet("color: #F44747;");
	}
}

void SettingsDialog::onLinkDocClicked()
{
	QDesktopServices::openUrl(QUrl("https://console.cloud.google.com/apis/credentials"));
}

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
