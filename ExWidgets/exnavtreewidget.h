#pragma once

#include <QScopedPointer>
#include <QTreeWidget>
#include <QVariantAnimation>

#include "exwidgets_global.h"

class ExNavTreeWidgetPrivate;

/**
 * @brief The ExNavTreeWidget class
 */
class EXWIDGETS_EXPORT ExNavTreeWidget : public QTreeWidget
{
    Q_OBJECT
    Q_PROPERTY(bool navigationExpanded READ navigationExpanded WRITE setNavigationExpanded)
    Q_PROPERTY(int compactWidth READ compactWidth WRITE setCompactWidth)
    Q_PROPERTY(int expandedWidth READ expandedWidth WRITE setExpandedWidth)
    Q_PROPERTY(bool autoHeightByItemsEnabled READ isAutoHeightByItemsEnabled WRITE setAutoHeightByItemsEnabled)

public:
    explicit ExNavTreeWidget(QWidget *parent = nullptr);
    ~ExNavTreeWidget() override;

    void setCompactWidth(int width);
    int compactWidth() const;
    void setExpandedWidth(int width);
    int expandedWidth() const;

    QTreeWidgetItem *addNavigationItem(const QString &text, int pageIndex, const QString &iconCode);
    void configureNavigationItem(QTreeWidgetItem *item, const QString &text, int pageIndex, const QString &iconCode = QString());
    void refreshNavigationIcons();

    void setNavigationExpanded(bool expanded, bool animated = true);
    bool navigationExpanded() const;
    void toggleNavigationMode();

    void setAutoHeightByItemsEnabled(bool enabled);
    bool isAutoHeightByItemsEnabled() const;
    // Compatibility alias for previous naming.
    void setFixedHeightByItems(bool enabled);

signals:
    void pageIndexChanged(int pageIndex);

protected:
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void handleItemSelection(QTreeWidgetItem *current);
    void updateFixedHeight();
    void updateNavigationItemIcon(QTreeWidgetItem *item);
    void updateNavigationItemText(QTreeWidgetItem *item, bool expanded);
    void updateNavigationItemExpansion(QTreeWidgetItem *item, bool expanded);
    void updateNavigationItemVisibilityForDepth(QTreeWidgetItem *item, int visibleDepth, int currentDepth = 0);
    void updateNavigationViewByWidth(int width);

private:
    Q_DECLARE_PRIVATE(ExNavTreeWidget)
    QScopedPointer<ExNavTreeWidgetPrivate> d_ptr;
};
