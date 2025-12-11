#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QDateTime>

#include <ILogger.hpp>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class StreamSegmenterDock : public QDockWidget, public Logger::ILogger {
	Q_OBJECT

public:
	explicit StreamSegmenterDock(std::shared_ptr<Logger::ILogger> logger, QWidget *parent = nullptr);
	~StreamSegmenterDock() override = default;

	void setSystemStatus(const QString &statusText, const QString &colorCode);
	void setNextSegmentationTime(const QDateTime &time);
	void setTimeRemaining(int remainingSeconds);

	void updateCurrentStream(const QString &title, const QString &status, const QString &url = "");
	void updateNextStream(const QString &title, const QString &status, const QString &url = "");

protected:
	void log(LogLevel level, std::string_view message) const noexcept override;
	const char *getPrefix() const noexcept override { return ""; }

signals:
	void logRequest(int level, const QString &message);

private slots:
	void onLogRequest(int level, const QString &message);
	void onStartClicked();
	void onStopClicked();
	void onSegmentNowClicked();
	void onSettingsClicked();
	void onLinkButtonClicked();

private:
	void setupUi();
	void updateMonitorLabel();

	std::shared_ptr<Logger::ILogger> logger_;

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
	QToolButton *const currentLinkButton_; // 【修正】Btn -> Button

	QWidget *const nextContainer_;
	QVBoxLayout *const nextLayout_;
	QLabel *const nextRoleLabel_;
	QLabel *const nextStatusLabel_;
	QLabel *const nextTitleLabel_;
	QToolButton *const nextLinkButton_; // 【修正】Btn -> Button

	// 4. Log Section
	QGroupBox *const logGroup_;
	QVBoxLayout *const logLayout_;
	QTextEdit *const consoleView_;

	// 5. Bottom Controls
	QVBoxLayout *const bottomControlLayout_;
	QPushButton *const settingsButton_;
	QPushButton *const segmentNowButton_; // 【修正】Btn -> Button
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
