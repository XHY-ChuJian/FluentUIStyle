#pragma once

#include <QWidget>
#include <QColor>
#include <QList>
#include <QToolButton>

#include "exwidgetsframeless_global.h"

class QAction;
class QButtonGroup;
class QLineEdit;
class QMainWindow;
class QLabel;

class EXWIDGETS_FRAMELESS_EXPORT FluentAccentColorButton : public QToolButton
{
    Q_OBJECT

public:
    explicit FluentAccentColorButton(QWidget *parent = nullptr);
    explicit FluentAccentColorButton(const QColor &color, QWidget *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

    bool isDefaultColor() const;
    void setIsDefaultColor(bool isDefault);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color;
    bool m_isDefault{false};
};

class EXWIDGETS_FRAMELESS_EXPORT FluentTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit FluentTitleBar(QMainWindow *window);

    QList<QToolButton *> accentButtons() const;
    QColor currentAccentColor() const;
    void setAccentColor(const QColor &color);
    void setAccentColors(const QList<QColor> &colors);

    QToolButton *themeButton() const;
    QToolButton *pinButton() const;
    QToolButton *minButton() const;
    QToolButton *maxButton() const;
    QToolButton *closeButton() const;
    QLineEdit *searchLineEdit() const;

    void setThemeDark(bool dark);
    void setPinned(bool pinned);

signals:
    void accentColorChanged(const QColor &color);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateTitle();
    void updateIcon();
    void updateMaxButton();
    void updateThemeButton();
    void updatePinButton();
    void updateAccentButtons();
    void onAccentButtonClicked(int id);

    QMainWindow *m_window{nullptr};
    QLabel *m_iconLabel{nullptr};
    QLabel *m_titleLabel{nullptr};
    QList<QColor> m_accentColors;
    QList<QToolButton *> m_accentButtons;
    QButtonGroup *m_accentButtonGroup{nullptr};
    int m_currentAccentIndex{0};
    QToolButton *m_themeButton{nullptr};
    QToolButton *m_pinButton{nullptr};
    QToolButton *m_minButton{nullptr};
    bool m_themeDark{false};
    bool m_pinned{false};
    QToolButton *m_maxButton{nullptr};
    QToolButton *m_closeButton{nullptr};
    QLineEdit *m_searchLineEdit{nullptr};
    QAction *m_searchAction{nullptr};
};
