#include "common/fluenthelpers.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QWidget>

namespace ClockUi
{
QFont fluentIconFont(int pixelSize)
{
    QFont font(QStringLiteral("Segoe Fluent Icons"));
    font.setPixelSize(pixelSize);
    return font;
}

QIcon fluentIcon(const QString& glyph, const QPalette& palette, int pixelSize)
{
    const int canvasSize = pixelSize + 10;
    QPixmap pixmap(canvasSize, canvasSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setFont(fluentIconFont(pixelSize));
    painter.setPen(palette.color(QPalette::Text));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);
    return QIcon(pixmap);
}

QLabel* createPageTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setPixelSize(36);
    font.setWeight(QFont::Light);
    label->setFont(font);
    return label;
}

QLabel* createSectionTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setPixelSize(18);
    font.setWeight(QFont::DemiBold);
    label->setFont(font);
    return label;
}

QVBoxLayout* createPageLayout(QWidget* page)
{
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(48, 34, 48, 34);
    layout->setSpacing(20);
    return layout;
}

QWidget* createEmptyState(const QString& icon,
                          const QString& title,
                          const QString& description,
                          QWidget* parent)
{
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    auto* iconLabel = new QLabel(icon, container);
    iconLabel->setFont(fluentIconFont(56));
    iconLabel->setAlignment(Qt::AlignCenter);

    auto* titleLabel = new QLabel(title, container);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(22);
    titleFont.setWeight(QFont::DemiBold);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* descriptionLabel = new QLabel(description, container);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setProperty("subtitle", true);

    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    return container;
}

QString formatDuration(qint64 milliseconds, bool includeHundredths)
{
    const qint64 safeMilliseconds = qMax<qint64>(0, milliseconds);
    const qint64 totalSeconds = includeHundredths
                                    ? safeMilliseconds / 1000
                                    : (safeMilliseconds + 999) / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 seconds = totalSeconds % 60;

    QString value = QStringLiteral("%1:%2:%3")
                        .arg(hours, 2, 10, QLatin1Char('0'))
                        .arg(minutes, 2, 10, QLatin1Char('0'))
                        .arg(seconds, 2, 10, QLatin1Char('0'));
    if (includeHundredths)
    {
        value += QStringLiteral(".%1")
                     .arg((safeMilliseconds / 10) % 100,
                          2,
                          10,
                          QLatin1Char('0'));
    }
    return value;
}
}

FluentRoundButton::FluentRoundButton(const QString& glyph,
                                     bool accent,
                                     QWidget* parent)
    : QAbstractButton(parent)
    , m_glyph(glyph)
    , m_accent(accent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(40, 40);
    setAccessibleName(glyph);
}

QSize FluentRoundButton::sizeHint() const
{
    return QSize(40, 40);
}

void FluentRoundButton::setGlyph(const QString& glyph)
{
    if (m_glyph == glyph)
        return;
    m_glyph = glyph;
    update();
}

void FluentRoundButton::setAccent(bool accent)
{
    if (m_accent == accent)
        return;
    m_accent = accent;
    update();
}

void FluentRoundButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor fill = m_accent ? palette().color(QPalette::Highlight)
                           : palette().color(QPalette::Button);
    if (!isEnabled())
        fill.setAlpha(70);
    else if (isDown())
        fill = fill.darker(118);
    else if (underMouse())
        fill = fill.lighter(112);

    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawEllipse(rect().adjusted(1, 1, -1, -1));

    QColor foreground = m_accent ? palette().color(QPalette::HighlightedText)
                                 : palette().color(QPalette::ButtonText);
    if (!isEnabled())
        foreground.setAlpha(90);
    painter.setPen(foreground);
    painter.setFont(ClockUi::fluentIconFont(17));
    painter.drawText(rect(), Qt::AlignCenter, m_glyph);
}
