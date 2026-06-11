#pragma once

#include <QWidget>

class QAction;
class QLineEdit;
class QMainWindow;
class QLabel;
class QToolButton;

class FluentTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit FluentTitleBar(QMainWindow *window);

    QToolButton *themeButton() const;
    QToolButton *pinButton() const;
    QToolButton *minButton() const;
    QToolButton *maxButton() const;
    QToolButton *closeButton() const;
    QLineEdit *searchLineEdit() const;

    void setThemeDark(bool dark);
    void setPinned(bool pinned);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateTitle();
    void updateIcon();
    void updateMaxButton();
    void updateThemeButton();
    void updatePinButton();

    QMainWindow *m_window{nullptr};
    QLabel *m_iconLabel{nullptr};
    QLabel *m_titleLabel{nullptr};
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
