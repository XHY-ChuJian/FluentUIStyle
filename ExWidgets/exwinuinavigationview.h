#pragma once

#include <QScopedPointer>
#include <QWidget>

#include "exwidgets_global.h"

class QStackedWidget;
class QTreeWidgetItem;
class QString;
class ExNavTreeWidget;
class ExWinUINavigationViewPrivate;

/**
 * WinUI3-style navigation pane: main menu (stretch) + horizontal rule + footer items.
 * Owns two ExNavTreeWidget instances, wires peers for cross-pane indicator animation,
 * and mirrors compact/expanded mode on both.
 */
class EXWIDGETS_EXPORT ExWinUINavigationView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool navigationExpanded READ navigationExpanded WRITE setNavigationExpanded)

public:
    explicit ExWinUINavigationView(QWidget *parent = nullptr);
    ~ExWinUINavigationView() override;

    ExNavTreeWidget *mainNavView() const;
    ExNavTreeWidget *footerNavView() const;

    void setStackedWidget(QStackedWidget *stack);
    QStackedWidget *stackedWidget() const;
    bool setSelectedPageIndex(int pageIndex);
    int selectedPageIndex() const;

    void setNavigationExpanded(bool expanded, bool animated = true);
    bool navigationExpanded() const;
    QTreeWidgetItem *addNavigationItem(const QString &text, int pageIndex, const QString &iconCode = QString());
    QTreeWidgetItem *addMainNavigationItem(const QString &text, int pageIndex, const QString &iconCode = QString());
    QTreeWidgetItem *addFooterNavigationItem(const QString &text, int pageIndex, const QString &iconCode = QString());
    void setCurrentMainItem(QTreeWidgetItem *item);
    void clearFooterSelection();

signals:
    void pageIndexChanged(int pageIndex);

private:
    void handleCrossViewSelection(ExNavTreeWidget *activatedNav, ExNavTreeWidget *peerNav, QTreeWidgetItem *current);

    Q_DECLARE_PRIVATE(ExWinUINavigationView)
    QScopedPointer<ExWinUINavigationViewPrivate> d_ptr;
};
