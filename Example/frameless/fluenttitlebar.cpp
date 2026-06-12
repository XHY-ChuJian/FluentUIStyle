#include "fluenttitlebar.h"

#include "qlineedit.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>

namespace {

constexpr QChar kChromeMinimize(0xE921);
constexpr QChar kChromeMaximize(0xE922);
constexpr QChar kChromeRestore(0xE923);
constexpr QChar kChromeClose(0xE8BB);
constexpr QChar kChromeTheme(0xE706);
constexpr QChar kChromePin(0xE718);
constexpr QChar kChromePinned(0xE77A);
constexpr QChar kChromeSearch(0xE721);

QFont captionIconFont(int pixelSize = 11)
{
    QFont font(QStringLiteral("Segoe Fluent Icons"));
    font.setPixelSize(pixelSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QToolButton *createCaptionButton(QWidget *parent, const QString &objectName, const QChar &glyph, int width = 46, int pixelSize = 11)
{
    auto *button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setFont(captionIconFont(pixelSize));
    button->setText(QString(glyph));
    button->setFixedSize(width, 40);
    return button;
}

QIcon searchIcon(bool darkTheme)
{
    constexpr int iconSize = 32;
    QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
    iconFont.setPixelSize(iconSize);

    QPixmap pixmap(iconSize, iconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setFont(iconFont);
    painter.setPen(darkTheme ? Qt::white : Qt::black);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString(kChromeSearch));

    return QIcon(pixmap);
}

} // namespace

FluentTitleBar::FluentTitleBar(QMainWindow *window)
    : QWidget(window)
    , m_window(window)
{
    setObjectName(QStringLiteral("fluent-title-bar"));
    setFixedHeight(40);
    setAttribute(Qt::WA_StyledBackground, true);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(16, 16);
    m_iconLabel->setScaledContents(true);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("fluent-title-label"));

    m_themeButton = createCaptionButton(this, QStringLiteral("win_caption_theme"), kChromeTheme, 40, 16);
    m_pinButton = createCaptionButton(this, QStringLiteral("win_caption_pin"), kChromePin, 40, 16);
    m_pinButton->setCheckable(true);

    m_minButton = createCaptionButton(this, QStringLiteral("win_caption_minimize"), kChromeMinimize);
    m_maxButton = createCaptionButton(this, QStringLiteral("win_caption_maximize"), kChromeMaximize);
    m_maxButton->setCheckable(true);
    m_closeButton = createCaptionButton(this, QStringLiteral("win_caption_close"), kChromeClose);

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setMinimumWidth(300);
    m_searchLineEdit->setPlaceholderText(tr("搜索..."));
    m_searchLineEdit->setClearButtonEnabled(true);
    m_searchAction = m_searchLineEdit->addAction(searchIcon(m_themeDark), QLineEdit::TrailingPosition);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_titleLabel);
    layout->addStretch();

    layout->addWidget(m_searchLineEdit, 0, Qt::AlignCenter);
    layout->addStretch();

    layout->addWidget(m_themeButton);
    layout->addWidget(m_pinButton);
    layout->addWidget(m_minButton);
    layout->addWidget(m_maxButton);
    layout->addWidget(m_closeButton);

    connect(m_minButton, &QToolButton::clicked, m_window, &QWidget::showMinimized);
    connect(m_maxButton, &QToolButton::clicked, m_window, [this]() {
        if (m_window->isMaximized())
        {
            m_window->showNormal();
        }
        else
        {
            m_window->showMaximized();
        }
    });
    connect(m_closeButton, &QToolButton::clicked, m_window, &QWidget::close);
    connect(m_pinButton, &QToolButton::toggled, this, [this]() {
        updatePinButton();
    });

    updateTitle();
    updateIcon();
    updateMaxButton();
    updateThemeButton();
    updatePinButton();
    m_window->installEventFilter(this);
}

QToolButton *FluentTitleBar::themeButton() const
{
    return m_themeButton;
}

QToolButton *FluentTitleBar::pinButton() const
{
    return m_pinButton;
}

QToolButton *FluentTitleBar::minButton() const
{
    return m_minButton;
}

QToolButton *FluentTitleBar::maxButton() const
{
    return m_maxButton;
}

QToolButton *FluentTitleBar::closeButton() const
{
    return m_closeButton;
}

QLineEdit *FluentTitleBar::searchLineEdit() const
{
    return m_searchLineEdit;
}

void FluentTitleBar::setThemeDark(bool dark)
{
    if (m_themeDark == dark)
    {
        return;
    }

    m_themeDark = dark;
    updateThemeButton();
    if (m_searchAction)
    {
        m_searchAction->setIcon(searchIcon(m_themeDark));
    }
}

void FluentTitleBar::setPinned(bool pinned)
{
    m_pinned = pinned;
    if (m_pinButton->isChecked() != pinned)
    {
        m_pinButton->setChecked(pinned);
    }
    else
    {
        updatePinButton();
    }
}

bool FluentTitleBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_window)
    {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type())
    {
    case QEvent::WindowIconChange:
        updateIcon();
        break;
    case QEvent::WindowTitleChange:
        updateTitle();
        break;
    case QEvent::WindowStateChange:
        updateMaxButton();
        break;
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void FluentTitleBar::updateTitle()
{
    m_titleLabel->setText(m_window->windowTitle());
}

void FluentTitleBar::updateIcon()
{
    QIcon icon = m_window->windowIcon();
    if (icon.isNull())
    {
        icon = QApplication::windowIcon();
    }
    if (icon.isNull())
    {
        icon = QIcon(QStringLiteral(":/appicon.ico"));
    }
    if (icon.isNull())
    {
        m_iconLabel->clear();
        return;
    }

    m_iconLabel->setPixmap(icon.pixmap(16, 16));
}

void FluentTitleBar::updateMaxButton()
{
    const bool maximized = m_window->isMaximized();
    m_maxButton->setChecked(maximized);
    m_maxButton->setText(maximized ? QString(kChromeRestore) : QString(kChromeMaximize));
}

void FluentTitleBar::updateThemeButton()
{
    m_themeButton->setText(QString(kChromeTheme));
    m_themeButton->setToolTip(m_themeDark ? QObject::tr("切换到浅色主题") : QObject::tr("切换到暗色主题"));
}

void FluentTitleBar::updatePinButton()
{
    const bool pinned = m_pinButton->isChecked();
    m_pinned = pinned;
    m_pinButton->setText(pinned ? QString(kChromePinned) : QString(kChromePin));
    m_pinButton->setToolTip(pinned ? QObject::tr("取消置顶") : QObject::tr("置顶窗口"));
    m_pinButton->update();
}
