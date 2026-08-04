#include "pages/settingspage.h"

#include "common/fluenthelpers.h"
#include "widgets/cardwidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
CardWidget* createSettingsRow(const QString& glyph,
                              const QString& title,
                              const QString& description,
                              QWidget* trailing,
                              QWidget* parent)
{
    auto* card = new CardWidget(parent);
    card->setMinimumHeight(84);
    auto* row = new QHBoxLayout(card);
    row->setContentsMargins(22, 12, 20, 12);
    row->setSpacing(18);

    auto* icon = new QLabel(glyph, card);
    icon->setFont(ClockUi::fluentIconFont(24));
    icon->setFixedWidth(28);
    icon->setAlignment(Qt::AlignCenter);

    auto* titleLabel = new QLabel(title, card);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    auto* descriptionLabel = new QLabel(description, card);
    QFont descriptionFont = descriptionLabel->font();
    descriptionFont.setPixelSize(12);
    descriptionLabel->setFont(descriptionFont);
    descriptionLabel->setProperty("subtitle", true);

    auto* textLayout = new QVBoxLayout;
    textLayout->setSpacing(2);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descriptionLabel);

    row->addWidget(icon);
    row->addLayout(textLayout, 1);
    if (trailing)
        row->addWidget(trailing);
    return card;
}

QCheckBox* createSwitch(bool checked, QWidget* parent)
{
    auto* checkBox = new QCheckBox(parent);
    checkBox->setProperty("isSwitchButton", true);
    checkBox->setChecked(checked);
    return checkBox;
}

bool systemUsesDarkTheme()
{
#ifdef Q_OS_WIN
    QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return false;
#endif
}

void applyFluentStyle(bool dark)
{
    qApp->setProperty("_q_colorscheme", dark ? 1 : 0);
    qApp->setProperty("_q_themestyle", 0);
    qApp->setStyle(QStringLiteral("FluentUI3"));
}
}

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    QFont settingsFont = font();
    settingsFont.setPixelSize(13);
    setFont(settingsFont);

    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget(scrollArea);
    auto* pageLayout = ClockUi::createPageLayout(content);

    auto* pageTitle = ClockUi::createPageTitle(tr("设置"), content);
    QFont pageTitleFont = pageTitle->font();
    pageTitleFont.setPixelSize(31);
    pageTitle->setFont(pageTitleFont);
    pageLayout->addWidget(pageTitle);

    const auto createSettingsSectionTitle = [content](const QString& text)
    {
        auto* title = ClockUi::createSectionTitle(text, content);
        QFont font = title->font();
        font.setPixelSize(15);
        title->setFont(font);
        return title;
    };

    pageLayout->addWidget(createSettingsSectionTitle(tr("账户")));
    auto* loginButton = new QPushButton(tr("登录"), content);
    loginButton->setEnabled(false);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE77B"),
        tr("Microsoft 帐户"),
        tr("登录后可在设备间同步闹钟和计时器"),
        loginButton,
        content));

    pageLayout->addSpacing(8);
    pageLayout->addWidget(createSettingsSectionTitle(tr("计时器与闹钟")));
    auto* timerSound = createSwitch(true, content);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE916"),
        tr("计时结束声音"),
        tr("计时器到达零时播放提示音"),
        timerSound,
        content));

    auto* alarmSound = createSwitch(true, content);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE7ED"),
        tr("闹钟声音"),
        tr("已启用的闹钟会播放系统提示音"),
        alarmSound,
        content));

    pageLayout->addSpacing(8);
    pageLayout->addWidget(createSettingsSectionTitle(tr("常规")));
    auto* themeCombo = new QComboBox(content);
    themeCombo->addItems({tr("深色"), tr("浅色"), tr("使用系统设置")});
    themeCombo->setMinimumWidth(160);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE790"),
        tr("应用程序主题"),
        tr("选择时钟使用的颜色模式"),
        themeCombo,
        content));

    auto* notificationButton = new QPushButton(tr("打开系统设置"), content);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE7ED"),
        tr("通知"),
        tr("更改 Windows 通知设置"),
        notificationButton,
        content));

    auto* clearButton = new QPushButton(tr("清除历史记录"), content);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE74D"),
        tr("隐私"),
        tr("清除本地保存的计时和计次历史"),
        clearButton,
        content));

    pageLayout->addSpacing(8);
    pageLayout->addWidget(createSettingsSectionTitle(tr("关于")));
    auto* versionLabel = new QLabel(QStringLiteral("0.1.0"), content);
    pageLayout->addWidget(createSettingsRow(
        QStringLiteral("\uE946"),
        tr("Win11 Clock"),
        tr("由 FluentUI3Style 驱动的 Qt Widgets 示例"),
        versionLabel,
        content));
    pageLayout->addStretch();

    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);

    connect(themeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index)
            {
                if (index == 0)
                    applyFluentStyle(true);
                else if (index == 1)
                    applyFluentStyle(false);
                else
                    applyFluentStyle(systemUsesDarkTheme());
                emit appearanceChanged();
            });

    connect(notificationButton,
            &QPushButton::clicked,
            this,
            []
            {
#ifdef Q_OS_WIN
                QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:notifications")));
#endif
            });

    connect(clearButton,
            &QPushButton::clicked,
            this,
            []
            {
                QSettings settings;
                settings.clear();
            });
}
