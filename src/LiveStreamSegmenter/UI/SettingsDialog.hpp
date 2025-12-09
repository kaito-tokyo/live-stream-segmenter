#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QJsonObject>

// Authモジュール
#include "../Auth/GoogleAuthManager.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

// --- JsonDropArea (変更なし) ---
class JsonDropArea : public QLabel {
	Q_OBJECT
public:
	explicit JsonDropArea(QWidget *parent = nullptr);
signals:
	void fileDropped(const QString &filePath);
	void clicked();

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
};

// --- SettingsDialog ---
class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog() override = default;

private slots:
	// UI Slots
	void onJsonFileSelected(const QString &filePath);
	void onAreaClicked();
	void onLoadJsonClicked();
	void onSaveClicked();
	void onAuthClicked();
	void onLinkDocClicked();

	// AuthManagerからのシグナル
	void onAuthStateChanged();
	void onLoginStatusChanged(const QString &status);
	void onLoginError(const QString &message);

private:
	void setupUi();
	void initializeData();
	void updateAuthUI(); // 認証状態に基づいて表示を更新

	// Helpers (CredentialsはUI側で管理し、AuthManagerに渡す)
	QString getObsConfigPath(const QString &filename) const;
	bool saveCredentialsToStorage(const QString &clientId, const QString &clientSecret);
	QJsonObject loadCredentialsFromStorage();
	bool parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret);

	// Data
	QString tempClientId_;
	QString tempClientSecret_;

	// Auth Manager (司令塔)
	Auth::GoogleAuthManager *authManager_;

	// UI Components
	JsonDropArea *dropArea_;
	QLineEdit *clientIdDisplay_;
	QLineEdit *clientSecretDisplay_;
	QPushButton *loadJsonButton_;
	QPushButton *saveButton_;
	QPushButton *authButton_;
	QLabel *statusLabel_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
