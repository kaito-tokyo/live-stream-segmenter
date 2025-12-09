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
#include <QComboBox>          // 追加

// Authモジュール
#include "../Auth/GoogleAuthManager.hpp"
#include "../API/YouTubeTypes.hpp" // データ型

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
    // --- Existing Slots ---
    void onJsonFileSelected(const QString &filePath);
    void onAreaClicked();
    void onLoadJsonClicked();
    void onSaveClicked();
    void onAuthClicked();
    
    // Auth Signals
    void onAuthStateChanged();
    void onLoginStatusChanged(const QString &status);
    void onLoginError(const QString &message);

    // --- New Slots for Stream Key ---
    void onRefreshKeysClicked();           // Reloadボタン
    void onKeySelectionChanged(int index); // 選択変更
    void onLinkDocClicked(); // 【追加】 これが抜けていました

private:
    void setupUi();
    void initializeData();
    void updateAuthUI();
    
    // --- New Helper: UI更新 ---
    // 別スレッドから呼ばれるため、スレッドセーフに更新する実体
    void updateStreamKeyList(const std::vector<API::YouTubeStreamKey> &keys);

    // --- Helpers ---
    QString getObsConfigPath(const QString &filename) const;

    // Credentials
    bool saveCredentialsToStorage(const QString &clientId, const QString &clientSecret);
    QJsonObject loadCredentialsFromStorage(); 
    bool parseCredentialJson(const QByteArray &jsonData, QString &outId, QString &outSecret);

    // --- New Helpers: Stream Key Config ---
    // config.json に保存する
    void saveStreamKeySetting(const QString &id, const QString &streamName, const QString &title);
    QString loadSavedStreamKeyId();

    // Data
    QString tempClientId_;
    QString tempClientSecret_;

    // Auth
    Auth::GoogleAuthManager *authManager_;

    // UI Components
    JsonDropArea *dropArea_;
    QLineEdit *clientIdDisplay_;
    QLineEdit *clientSecretDisplay_;
    QPushButton *loadJsonButton_;
    QPushButton *saveButton_;
    QPushButton *authButton_;
    QLabel *statusLabel_;

    // --- New UI Components ---
    QComboBox *streamKeyCombo_;
    QPushButton *refreshKeysBtn_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
