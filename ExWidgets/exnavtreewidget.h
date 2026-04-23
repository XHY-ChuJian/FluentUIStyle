#pragma once

#include <QHash>
#include <QList>
#include <QTreeWidget>
#include <QVariantAnimation>

#include "exwidgets_global.h"

class EXWIDGETS_EXPORT ExNavTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ExNavTreeWidget( QWidget* parent = nullptr );

    void setCompactWidth( int width );
    void setExpandedWidth( int width );

    QTreeWidgetItem* addNavigationItem( const QString& text, int pageIndex, const QString& iconCode );
    void configureNavigationItem( QTreeWidgetItem* item, const QString& text, int pageIndex, const QString& iconCode = QString() );
    void refreshNavigationIcons();

    void setNavigationExpanded( bool expanded, bool animated = true );
    bool navigationExpanded() const;
    void toggleNavigationMode();

signals:
    void pageIndexChanged( int pageIndex );

protected:
    void changeEvent( QEvent* event ) override;
    void mousePressEvent( QMouseEvent* event ) override;

private:
    void updateNavigationItemIcon( QTreeWidgetItem* item );
    void updateNavigationItemText( QTreeWidgetItem* item, bool expanded );
    void updateNavigationItemExpansion( QTreeWidgetItem* item, bool expanded );
    void updateNavigationItemVisibilityForDepth( QTreeWidgetItem* item, int visibleDepth, int currentDepth = 0 );
    void updateNavigationHierarchyForMode( QTreeWidgetItem* item, bool expanded );
    void updateNavigationViewByWidth( int width );

private:
    QVariantAnimation* m_navigationWidthAnimation { nullptr };
    bool m_navigationExpanded { false };
    int m_navigationCompactWidth { 44 };
    int m_navigationExpandedWidth { 168 };
};
