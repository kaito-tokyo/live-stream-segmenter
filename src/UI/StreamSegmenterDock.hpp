#pragma once

#include <QDockWidget>
#include <QTextEdit>
#include <QLabel>
#include <QTreeWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QDateTime>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {

class StreamSegmenterDock : public QDockWidget {
    Q_OBJECT

public:
    explicit StreamSegmenterDock(QWidget *parent = nullptr);
    ~StreamSegmenterDock() override = default;

    // ログレベル定義
    enum class LogLevel { Info, Success, Warning, Error };
    
    // 外部（ロジック部）からログを追記するためのスレッドセーフなメソッド
    void appendLog(const QString &message, LogLevel level = LogLevel::Info);

    // ステータス更新用メソッド
    void updateNextSegmentationTime(const QDateTime &time);
    void updateTimeRemaining(const QString &timeString);
    void setSystemStatus(const QString &status, const QString &colorCode);

private:
    void setupUi();

    // UI Components
    QLabel *m_statusLabel;
    QLabel *m_nextSegmentationLabel;
    QLabel *m_timeRemainingLabel;
    
    QTreeWidget *m_scheduleTable;
    QTextEdit *m_consoleView;
    
    QCheckBox *m_autoSegmentCheck;
    QPushButton *m_segmentNowBtn;
};

} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
