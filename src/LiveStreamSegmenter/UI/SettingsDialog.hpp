#pragma once

#include <QDialog>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog() override = default;

private:
	void setupUi();
};

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
