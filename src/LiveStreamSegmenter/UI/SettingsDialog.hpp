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
#include <QComboBox>
#include <QTimer> // 追加

#include <memory>
#include <future>
#include <optional>

// Authモジュール
#include "../Auth/GoogleAuthManager.hpp"
#include "../Auth/GoogleOAuth2Flow.hpp" // 追加
#include "../API/YouTubeTypes.hpp"

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
	~SettingsDialog() override; // デストラクタの実装が必要（unique_ptrのため）

private slots:
	// --- Existing Slots ---
	void onJsonFileSelected(const QString &filePath);
	void onAreaClicked();
	void onLoadJsonClicked();
	void onSaveClicked();
	
	// Auth
	void onAuthClicked();
	void onAuthPollTimer(); // 追加: フロー完了監視用

	// Auth Signals (Managerからの通知)
	void onAuthStateChanged();
	void onLoginStatusChanged(const QString &status);
	void onLoginError(const QString &message);

	// Stream Key
	void onRefreshKeysClicked();
	void onKeySelectionChanged(int index);
	void onLinkDocClicked();

private:
	void setupUi();
	void initializeData();
	void updateAuthUI();

	void updateStreamKeyList(const std::vector<API::YouTubeStreamKey> &keys);

	// Helpers
	QString getObsConfigPath(const QString &filename) const;
	bool saveCredentialsToStorage(const QString &clientId, const QString &clientSecret);
	QJsonObject loadCredentialsFromStorage();
	bool parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret);

	void saveStreamKeySetting(const QString &id, const QString &streamName, const QString &title);
	QString loadSavedStreamKeyId();

	// Data
	QString tempClientId_;
	QString tempClientSecret_;

	// Managers
	Auth::GoogleAuthManager *authManager_;

	// --- New Auth Flow Management ---
	// SettingsDialogのライフサイクルとフローのライフサイクルを管理するため
	std::unique_ptr<Auth::GoogleOAuth2Flow> oauthFlow_;
	std::future<std::optional<Auth::GoogleTokenResponse>> pendingAuthFuture_;
	QTimer *authPollTimer_;

	// UI Components
	JsonDropArea *dropArea_;
	QLineEdit *clientIdDisplay_;
	QLineEdit *clientSecretDisplay_;
	QPushButton *loadJsonButton_;
	QPushButton *saveButton_;
	QPushButton *authButton_;
	QLabel *statusLabel_;

	QComboBox *streamKeyCombo_;
	QPushButton *refreshKeysBtn_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
