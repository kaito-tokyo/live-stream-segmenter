#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>
#include <QToolButton> // 【追加】アイコン的なボタン用
#include <QDateTime>
// #include <QEvent> // 削除

#include <ILogger.hpp>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class StreamSegmenterDock : public QDockWidget, public Logger::ILogger {
	Q_OBJECT

public:
	explicit StreamSegmenterDock(QWidget *parent = nullptr);
	~StreamSegmenterDock() override = default;

	void setSystemStatus(const QString &statusText, const QString &colorCode);
	void setNextSegmentationTime(const QDateTime &time);
	void setTimeRemaining(int remainingSeconds);

	void updateCurrentStream(const QString &title, const QString &status, const QString &url = "");
	void updateNextStream(const QString &title, const QString &status, const QString &url = "");

protected:
	void log(LogLevel level, std::string_view message) const noexcept override;
	const char *getPrefix() const noexcept override { return ""; }

	// eventFilter は削除

signals:
	void logRequest(int level, const QString &message);

private slots:
	void onLogRequest(int level, const QString &message);
	void onStartClicked();
	void onStopClicked();
	void onSegmentNowClicked();
	void onSettingsClicked();

	// 【追加】リンクボタンクリック時の処理
	void onLinkButtonClicked();

private:
	void setupUi();
	void updateMonitorLabel();

	// Data Cache
	QString currentStatusText_;
	QString currentStatusColor_;
	QString currentNextTimeText_;
	QString currentTimeRemainingText_;

	// --- UI Components ---

	QWidget *const mainWidget_;
	QWidget *const titleBarWidget_;
	QVBoxLayout *const mainLayout_;

	// 1. Top Controls
	QHBoxLayout *const topControlLayout_;
	QPushButton *const startButton_;
	QPushButton *const stopButton_;

	// 2. Status Section
	QGroupBox *const statusGroup_;
	QVBoxLayout *const statusLayout_;
	QLabel *const monitorLabel_;

	// 3. Schedule Section
	QGroupBox *const scheduleGroup_;
	QVBoxLayout *const scheduleLayout_;

	QWidget *const currentContainer_;
	QVBoxLayout *const currentLayout_;
	QLabel *const currentRoleLabel_;
	QLabel *const currentStatusLabel_;
	QLabel *const currentTitleLabel_;
	QToolButton *const currentLinkBtn_; // 【追加】

	QWidget *const nextContainer_;
	QVBoxLayout *const nextLayout_;
	QLabel *const nextRoleLabel_;
	QLabel *const nextStatusLabel_;
	QLabel *const nextTitleLabel_;
	QToolButton *const nextLinkBtn_; // 【追加】

	// 4. Log Section
	QGroupBox *const logGroup_;
	QVBoxLayout *const logLayout_;
	QTextEdit *const consoleView_;

	// 5. Bottom Controls
	QVBoxLayout *const bottomControlLayout_;
	QPushButton *const settingsButton_;
	QPushButton *const segmentNowBtn_;
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
