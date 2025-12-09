#include "SettingsDialog.hpp"
#include <QVBoxLayout>
#include <QLabel>

namespace KaitoTokyo {
namespace LiveStreamSegmenter {
namespace UI {

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setupUi();
}

void SettingsDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    
    // 将来ここに設定項目を追加します
    auto *placeholderLabel = new QLabel(tr("Settings will be implemented here."), this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    
    mainLayout->addWidget(placeholderLabel);
    
    // モック用の仮サイズ
    resize(400, 300);
}

} // namespace UI
} // namespace LiveStreamSegmenter
} // namespace KaitoTokyo
