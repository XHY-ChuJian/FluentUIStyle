#pragma once

#include <QAbstractButton>
#include <QIcon>
#include <QLabel>

class QHBoxLayout;
class QVBoxLayout;
class QWidget;

namespace ClockUi
{
QFont fluentIconFont(int pixelSize = 18);
QIcon fluentIcon(const QString& glyph,
                 const QPalette& palette,
                 int pixelSize = 20);
QLabel* createPageTitle(const QString& text, QWidget* parent = nullptr);
QLabel* createSectionTitle(const QString& text, QWidget* parent = nullptr);
QVBoxLayout* createPageLayout(QWidget* page);
QWidget* createEmptyState(const QString& icon,
                          const QString& title,
                          const QString& description,
                          QWidget* parent = nullptr);
QString formatDuration(qint64 milliseconds, bool includeHundredths = false);
}

class FluentRoundButton final : public QAbstractButton
{
    Q_OBJECT

public:
    explicit FluentRoundButton(const QString& glyph,
                               bool accent,
                               QWidget* parent = nullptr);

    QSize sizeHint() const override;
    void setGlyph(const QString& glyph);
    void setAccent(bool accent);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_glyph;
    bool m_accent{false};
};
