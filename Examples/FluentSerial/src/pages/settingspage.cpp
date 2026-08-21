#include "settingspage.h"

#ifndef WIN32
#include "fluentui3style.h"
#endif

#include <QApplication>
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
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    // 1. 个性化与外观
    auto *themeCard = createCardFrame(container);
    auto *themeLayout = new QVBoxLayout(themeCard);
    themeLayout->setContentsMargins(14, 12, 14, 12);
    themeLayout->setSpacing(12);

    themeLayout->addWidget(createCardHeader(QStringLiteral("🎨 外观与主题"), themeCard));

    auto *tRow = new QHBoxLayout();
    tRow->addWidget(new QLabel(QStringLiteral("应用主题模式:"), themeCard));
    tRow->addStretch();

    m_themeCombo = new QComboBox(themeCard);
    m_themeCombo->addItem(QStringLiteral("浅色 (Light)"), 0);
    m_themeCombo->addItem(QStringLiteral("深色 (Dark)"), 1);
    m_themeCombo->setCurrentIndex(qApp->property("_q_colorscheme").toInt() == 1 ? 1 : 0);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);
    tRow->addWidget(m_themeCombo);

    themeLayout->addLayout(tRow);
    mainLayout->addWidget(themeCard);

    // 2. 关于信息
    auto *aboutCard = createCardFrame(container);
    auto *aboutLayout = new QVBoxLayout(aboutCard);
    aboutLayout->setContentsMargins(14, 12, 14, 12);
    aboutLayout->setSpacing(8);

    aboutLayout->addWidget(createCardHeader(QStringLiteral("ℹ️ 关于 FluentSerial"), aboutCard));

    auto *descLabel = new QLabel(
        QStringLiteral(
            "<b>FluentSerial - 现代串口调试与嵌入式协议工作台</b><br>"
            "版本: v1.0.0<br>"
            "基于 Qt6、FluentUI3 现代设计语言与 ExWidgets 组件库构建。<br>"
            "集串口数据高速收发、自动化指令序列、实时波形示波器、多功能 CRC/Modbus 工具箱于一体。<br><br>"
            "Copyright © 2026. All rights reserved."
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
    qApp->setProperty("_q_colorscheme", val);
#ifdef WIN32
    qApp->setStyle(QStringLiteral("FluentUI3"));
#else
    qApp->setStyle(new FluentUI3Style);
#endif
    emit appearanceChanged();
}
