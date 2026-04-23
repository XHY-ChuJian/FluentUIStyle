#include "exnavtreewidget.h"

#include <climits>

#include <QApplication>
#include <QAbstractItemView>
#include <QEvent>
#include <QEasingCurve>
#include <QFont>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QMouseEvent>

namespace
{
    constexpr int NavigationPageRole = Qt::UserRole;
    constexpr int NavigationIconRole = Qt::UserRole + 1;
    constexpr int NavigationTextRole = Qt::UserRole + 2;
    constexpr int NavigationDetachChildrenRole = Qt::UserRole + 3;
    constexpr int NavigationWasExpandedRole = Qt::UserRole + 4;
    constexpr int NavigationWasSelectedItemRole = Qt::UserRole + 5;

    QIcon createNavigationIcon(const QString &unicode, const QColor &color)
    {
        if (unicode.isEmpty())
        {
            return QIcon();
        }

        constexpr int pixelSize = 25;
        QFont iconFont("Segoe Fluent Icons");
        iconFont.setPixelSize(pixelSize);

        QPixmap pixmap(30, 30);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        painter.setFont(iconFont);
        painter.setPen(color);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, unicode);

        return QIcon(pixmap);
    }
}

ExNavTreeWidget::ExNavTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setObjectName("ExNavTreeWidget");

    QFont navFont = font();
    navFont.setPixelSize(navFont.pixelSize() + 1);
    navFont.setHintingPreference(QFont::PreferFullHinting);

    setAnimated(true);
    setIconSize(QSize(20, 20));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFont(navFont);
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextElideMode(Qt::ElideRight);
    setStyleSheet("ExNavTreeWidget{background:transparent;}");
    setProperty("navigationViewIndicator", true);
    header()->setSectionResizeMode(0, QHeaderView::Fixed);
    setColumnWidth(0, m_navigationCompactWidth);
    setFixedWidth(m_navigationCompactWidth);

    m_navigationWidthAnimation = new QVariantAnimation(this);
    m_navigationWidthAnimation->setDuration(280);
    m_navigationWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_navigationWidthAnimation,
            &QVariantAnimation::valueChanged,
            this,
            [this](const QVariant &value)
            {
                updateNavigationViewByWidth(value.toInt());
            });
    connect(m_navigationWidthAnimation,
            &QVariantAnimation::finished,
            this,
            [this]()
            {
                const int targetWidth = m_navigationExpanded ? m_navigationExpandedWidth : m_navigationCompactWidth;
                updateNavigationViewByWidth(targetWidth);
            });

    connect(this,
            &QTreeWidget::currentItemChanged,
            this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *)
            {
                if (!current)
                {
                    return;
                }

                const QVariant pageIndex = current->data(0, NavigationPageRole);
                if (pageIndex.isValid())
                {
                    emit pageIndexChanged(pageIndex.toInt());
                }
            });
}

void ExNavTreeWidget::setCompactWidth(int width)
{
    m_navigationCompactWidth = qMax(1, width);
    if (!m_navigationExpanded)
    {
        m_navigationWidthAnimation->stop();
        updateNavigationViewByWidth(m_navigationCompactWidth);
    }
}

void ExNavTreeWidget::setExpandedWidth(int width)
{
    m_navigationExpandedWidth = qMax(m_navigationCompactWidth, width);
    if (m_navigationExpanded)
    {
        m_navigationWidthAnimation->stop();
        updateNavigationViewByWidth(m_navigationExpandedWidth);
    }
}

QTreeWidgetItem *ExNavTreeWidget::addNavigationItem(const QString &text, int pageIndex, const QString &iconCode)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    configureNavigationItem(item, text, pageIndex, iconCode);
    return item;
}

void ExNavTreeWidget::configureNavigationItem(QTreeWidgetItem *item, const QString &text, int pageIndex, const QString &iconCode)
{
    if (!item)
    {
        return;
    }

    item->setData(0, NavigationPageRole, pageIndex);
    item->setData(0, NavigationTextRole, text);
    item->setData(0, NavigationIconRole, iconCode);
    item->setToolTip(0, text);
    updateNavigationItemIcon(item);
    updateNavigationItemText(item, m_navigationExpanded);
}

void ExNavTreeWidget::refreshNavigationIcons()
{
    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        updateNavigationItemIcon(topLevelItem(i));
    }
}

void ExNavTreeWidget::setNavigationExpanded(bool expanded, bool animated)
{
    m_navigationExpanded = expanded;

    const int targetWidth = expanded ? m_navigationExpandedWidth : m_navigationCompactWidth;
    if (!animated || !m_navigationWidthAnimation)
    {
        updateNavigationViewByWidth(targetWidth);
        return;
    }

    const int currentWidth = width();
    if (currentWidth == targetWidth)
    {
        return;
    }

    m_navigationWidthAnimation->stop();
    m_navigationWidthAnimation->setStartValue(currentWidth);
    m_navigationWidthAnimation->setEndValue(targetWidth);
    m_navigationWidthAnimation->start();
}

bool ExNavTreeWidget::navigationExpanded() const
{
    return m_navigationExpanded;
}

void ExNavTreeWidget::toggleNavigationMode()
{
    setNavigationExpanded(!m_navigationExpanded, true);
}

void ExNavTreeWidget::changeEvent(QEvent *event)
{
    QTreeWidget::changeEvent(event);

    if (!event)
    {
        return;
    }

    switch (event->type())
    {
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::StyleChange:
        refreshNavigationIcons();
        break;
    default:
        break;
    }
}

void ExNavTreeWidget::updateNavigationItemIcon(QTreeWidgetItem *item)
{
    if (!item)
    {
        return;
    }

    const QString iconCode = item->data(0, NavigationIconRole).toString();
    item->setIcon(0, createNavigationIcon(iconCode, palette().color(QPalette::Text)));

    for (int i = 0; i < item->childCount(); ++i)
    {
        updateNavigationItemIcon(item->child(i));
    }
}

void ExNavTreeWidget::updateNavigationItemText(QTreeWidgetItem *item, bool expanded)
{
    if (!item)
    {
        return;
    }

    item->setText(0, expanded ? item->data(0, NavigationTextRole).toString() : QString());
    for (int i = 0; i < item->childCount(); ++i)
    {
        updateNavigationItemText(item->child(i), expanded);
    }
}

void ExNavTreeWidget::updateNavigationItemExpansion(QTreeWidgetItem *item, bool showText)
{
    if (!item)
    {
        return;
    }

    if (item->childCount() > 0)
    {
        if (!showText)
        {
            // 切换到图标模式：记录并折叠
            item->setData(0, NavigationWasExpandedRole, item->isExpanded());
            item->setExpanded(false);
        }
        else
        {
            // 切换回文本模式：恢复先前的折叠状态（默认不展开）
            QVariant wasExpanded = item->data(0, NavigationWasExpandedRole);
            item->setExpanded(wasExpanded.isValid() ? wasExpanded.toBool() : false);
            item->setData(0, NavigationWasExpandedRole, QVariant());
        }
    }

    for (int i = 0; i < item->childCount(); ++i)
    {
        updateNavigationItemExpansion(item->child(i), showText);
    }
}

void ExNavTreeWidget::updateNavigationItemVisibilityForDepth(QTreeWidgetItem *item, int visibleDepth, int currentDepth)
{
    if (!item)
    {
        return;
    }

    item->setHidden(currentDepth > visibleDepth);
    for (int i = 0; i < item->childCount(); ++i)
    {
        updateNavigationItemVisibilityForDepth(item->child(i), visibleDepth, currentDepth + 1);
    }
}

void ExNavTreeWidget::updateNavigationViewByWidth(int width)
{
    const int textSwitchWidth = m_navigationCompactWidth + (m_navigationExpandedWidth - m_navigationCompactWidth) * 2 / 3;
    const bool showText = width >= textSwitchWidth;
    const int visibleDepth = showText ? INT_MAX : 1;

    bool oldIconMode = property("navigationIconMode").toBool();
    bool willBeIconMode = !showText;
    bool modeFlipped = (oldIconMode != willBeIconMode);

    // 仅在真实发生模式翻转时处理
    if (modeFlipped)
    {
        if (willBeIconMode)
        {
            QTreeWidgetItem *current = currentItem();
            if (current && current->parent())
            {
                QTreeWidgetItem *topLevel = current;
                while (topLevel->parent())
                {
                    topLevel = topLevel->parent();
                }
                // 保存真实的子节点以便在恢复时重新选中
                topLevel->setData(0, NavigationWasSelectedItemRole, QVariant::fromValue(reinterpret_cast<quintptr>(current)));
                setCurrentItem(topLevel);
            }
        }
        else
        {
            // 切回展开模式时，检查当前的一级节点是否曾保留过选中的子节点记录
            QTreeWidgetItem *current = currentItem();
            if (current && !current->parent())
            {
                QVariant savedChild = current->data(0, NavigationWasSelectedItemRole);
                if (savedChild.isValid())
                {
                    auto *childToRestore = reinterpret_cast<QTreeWidgetItem *>(savedChild.value<quintptr>());
                    setCurrentItem(childToRestore);
                    current->setData(0, NavigationWasSelectedItemRole, QVariant()); // 清除记录
                }
            }
        }
    }

    setProperty("navigationIconMode", !showText);

    setUpdatesEnabled(false);
    setFixedWidth(width);

    const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    const int frameBorderWidth = this->frameWidth() * 2;
    const int safeColumnWidth = qMax(m_navigationCompactWidth, width - scrollBarExtent - frameBorderWidth);
    setColumnWidth(0, safeColumnWidth);

    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = topLevelItem(i);
        updateNavigationItemText(item, showText);
        updateNavigationItemVisibilityForDepth(item, visibleDepth);
        if (modeFlipped)
        {
            updateNavigationItemExpansion(item, showText);
        }
    }

    setUpdatesEnabled(true);
    viewport()->update();
}

void ExNavTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    setCurrentIndex(index);

    // 如果处于图标模式，由于不显示右侧箭头，所以整个 Item 区域的点击都作为选中处理，不拦截
    if (index.isValid() && model()->hasChildren(index) && !property("navigationIconMode").toBool())
    {
        QRect r = visualRect(index);
        bool isReverse = layoutDirection() == Qt::RightToLeft;
        // The arrow is drawn by the style at the rightmost 22 pixels.
        // We give it a generous click zone of 30 pixels.
        int arrowZone = 30;
        bool isArrowClick = isReverse ? (pos.x() < r.left() + arrowZone) : (pos.x() > r.right() - arrowZone);

        if (isArrowClick)
        {
            if (isExpanded(index))
                collapse(index);
            else
                expand(index);
            event->accept();
            return;
        }
    }

    QTreeWidget::mousePressEvent(event);
}
