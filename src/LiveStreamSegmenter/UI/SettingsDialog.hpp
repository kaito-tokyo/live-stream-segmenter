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

#include "OAuth2Handler.hpp" // 【追加】

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

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

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog() override = default;

private slots:
	void onJsonFileSelected(const QString &filePath);
	void onAreaClicked();
	void onLoadJsonClicked();
	void onSaveClicked();
	void onAuthClicked();
	void onLinkDocClicked();

	// Handlerからのシグナルを受けるスロット
	void onAuthStatusChanged(const QString &status);
	void onAuthSuccess(const QString &refreshToken, const QString &accessToken, const QString &email);
	void onAuthError(const QString &message);

private:
	void setupUi();
	void initializeData();

	// 【追加】この行が抜けていました！
	void updateAuthStatus(bool isConnected, const QString &accountName = "");

	// Logic Helpers
	QString getObsConfigPath(const QString &filename) const;
	bool saveCredentialsToStorage(const QString &clientId, const QString &clientSecret);
	QJsonObject loadCredentialsFromStorage();
	bool parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret);

	OAuth2Handler *oauthHandler_;

	// Data
	QString tempClientId_;
	QString tempClientSecret_;

	// UI Components
	JsonDropArea *dropArea_;
	QLineEdit *clientIdDisplay_;
	QLineEdit *clientSecretDisplay_;
	QPushButton *loadJsonButton_;
	QPushButton *saveButton_;
	QPushButton *authButton_;
	QLabel *statusLabel_;
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
