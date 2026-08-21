#include "settingspage.h"

#include "core/systemprovider.h"
#include "exexpander.h"

#ifndef WIN32
#include "fluentui3style.h"
#endif

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QFrame *createCardFrame(QWidget *parent = nullptr)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("MonitorCard"));
    frame->setStyleSheet(QStringLiteral(
        "QFrame#MonitorCard {"
        "  border: 1px solid rgba(128, 128, 128, 0.22);"
        "  border-radius: 8px;"
        "  background-color: palette(base);"
        "}"
    ));
    return frame;
}

QLabel *createCardHeader(const QString &title, QWidget *parent = nullptr)
{
    auto *label = new QLabel(title, parent);
    QFont f = label->font();
    f.setPixelSize(15);
    f.setBold(true);
    label->setFont(f);
    return label;
}

} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SettingsPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scrollArea);
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel(QStringLiteral("应用设置与偏好"), container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 外观设置卡片
    auto *themeCard = createCardFrame(container);
    auto *themeLayout = new QVBoxLayout(themeCard);
    themeLayout->setContentsMargins(18, 16, 18, 18);
    themeLayout->setSpacing(12);
    themeLayout->addWidget(createCardHeader(QStringLiteral("🎨 外观与主题模式"), themeCard));

    auto *tRow = new QHBoxLayout;
    tRow->addWidget(new QLabel(QStringLiteral("选择界面主题:"), themeCard));
    tRow->addStretch();
    m_themeCombo = new QComboBox(themeCard);
    m_themeCombo->addItem(QStringLiteral("浅色模式 (Light)"), 0);
    m_themeCombo->addItem(QStringLiteral("深色模式 (Dark)"), 1);
    m_themeCombo->addItem(QStringLiteral("跟随系统 (System)"), 2);
    m_themeCombo->setCurrentIndex(qApp->property("_q_colorscheme").toInt() == 1 ? 1 : 0);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);
    tRow->addWidget(m_themeCombo);
    themeLayout->addLayout(tRow);

    mainLayout->addWidget(themeCard);

    // 监控行为卡片
    auto *monCard = createCardFrame(container);
    auto *monLayout = new QVBoxLayout(monCard);
    monLayout->setContentsMargins(18, 16, 18, 18);
    monLayout->setSpacing(12);
    monLayout->addWidget(createCardHeader(QStringLiteral("⚙️ 监控与刷新行为"), monCard));

    auto *sRow = new QHBoxLayout;
    sRow->addWidget(new QLabel(QStringLiteral("默认采样刷新间隔:"), monCard));
    sRow->addStretch();
    m_intervalCombo = new QComboBox(monCard);
    m_intervalCombo->addItem(QStringLiteral("极速 (500ms)"), 500);
    m_intervalCombo->addItem(QStringLiteral("标准 (1秒)"), 1000);
    m_intervalCombo->addItem(QStringLiteral("节能 (2秒)"), 2000);
    m_intervalCombo->addItem(QStringLiteral("低频 (5秒)"), 5000);
    m_intervalCombo->setCurrentIndex(1);
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        SystemProvider::instance().setInterval(m_intervalCombo->itemData(idx).toInt());
    });
    sRow->addWidget(m_intervalCombo);
    monLayout->addLayout(sRow);

    mainLayout->addWidget(monCard);

    // 关于卡片
    auto *aboutCard = createCardFrame(container);
    auto *aboutLayout = new QVBoxLayout(aboutCard);
    aboutLayout->setContentsMargins(18, 16, 18, 18);
    aboutLayout->setSpacing(10);
    aboutLayout->addWidget(createCardHeader(QStringLiteral("ℹ️ 关于 FluentSysMon"), aboutCard));

    auto *descLabel = new QLabel(
        QStringLiteral(
            "<b>FluentSysMon 现代系统与硬件监控箱</b><br/>"
            "基于 Qt + FluentUI3Style + ExWidgets 构建的高性能、高颜值独立实用工具。<br/>"
            "版本: v1.0.0 (Release)<br/>"
            "开源协议: MIT License<br/>"
            "仓库主页: <a href='https://github.com/XHY-ChuJian/FluentUIStyle'>https://github.com/XHY-ChuJian/FluentUIStyle</a>"
        ),
        aboutCard
    );
    descLabel->setOpenExternalLinks(true);
    aboutLayout->addWidget(descLabel);

    mainLayout->addWidget(aboutCard);
    mainLayout->addStretch();

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void SettingsPage::setDarkTheme(bool isDark)
{
    m_themeCombo->blockSignals(true);
    m_themeCombo->setCurrentIndex(isDark ? 1 : 0);
    m_themeCombo->blockSignals(false);

    qApp->setProperty("_q_colorscheme", isDark ? 1 : 0);
#ifdef WIN32
    qApp->setStyle(QStringLiteral("FluentUI3"));
#else
    qApp->setStyle(new FluentUI3Style);
#endif
    emit appearanceChanged();
}

void SettingsPage::onThemeChanged(int index)
{
    int val = m_themeCombo->itemData(index).toInt();
    if (val == 2) {
        val = 0; // 默认浅色
    }
    qApp->setProperty("_q_colorscheme", val);
#ifdef WIN32
    qApp->setStyle(QStringLiteral("FluentUI3"));
#else
    qApp->setStyle(new FluentUI3Style);
#endif
    emit appearanceChanged();
}
