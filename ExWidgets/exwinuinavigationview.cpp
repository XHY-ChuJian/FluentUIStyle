#include "exwinuinavigationview.h"

#include "exnavtreewidget.h"

#include <QFont>
#include <QFrame>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr int NavigationPageRole = Qt::UserRole;
}

class ExWinUINavigationViewPrivate
{
public:
    QStackedWidget *stackedWidget{nullptr};
    ExNavTreeWidget *mainNav{nullptr};
    ExNavTreeWidget *footerNav{nullptr};
    QFrame *line{nullptr};
};

ExWinUINavigationView::ExWinUINavigationView(QWidget *parent)
    : QWidget(parent), d_ptr(new ExWinUINavigationViewPrivate)
{
    Q_D(ExWinUINavigationView);
    setObjectName(QStringLiteral("ExWinUINavigationView"));

    d->mainNav = new ExNavTreeWidget(this);
    d->mainNav->setObjectName(QStringLiteral("mainNavView"));

    d->line = new QFrame(this);
    d->line->setObjectName(QStringLiteral("line"));
    d->line->setFrameShape(QFrame::HLine);
    d->line->setFrameShadow(QFrame::Sunken);
    d->line->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    d->footerNav = new ExNavTreeWidget(this);
    d->footerNav->setObjectName(QStringLiteral("footerNavView"));
    d->footerNav->setAnimated(false);
    d->footerNav->setAutoHeightByItemsEnabled(true);

    d->mainNav->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    d->mainNav->setIndentation(20);
    QFont mainFont = d->mainNav->font();
    mainFont.setPointSize(10);
    d->mainNav->setFont(mainFont);

    d->footerNav->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->footerNav->setIndentation(20);
    d->footerNav->setFont(mainFont);

    auto *mainContainer = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(6, 6, 6, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(d->mainNav);

    auto *footerContainer = new QWidget(this);
    auto *footerLayout = new QVBoxLayout(footerContainer);
    footerLayout->setContentsMargins(6, 0, 6, 6);
    footerLayout->setSpacing(0);
    footerLayout->addWidget(d->footerNav);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(mainContainer, 1);
    lay->addWidget(d->line, 0);
    lay->addWidget(footerContainer, 0);

    connect(d->mainNav,
            &QTreeWidget::currentItemChanged,
            this,
            [this, d](QTreeWidgetItem *current, QTreeWidgetItem *)
            {
                handleCrossViewSelection(d->mainNav, d->footerNav, current);
            });
    connect(d->footerNav,
            &QTreeWidget::currentItemChanged,
            this,
            [this, d](QTreeWidgetItem *current, QTreeWidgetItem *)
            {
                handleCrossViewSelection(d->footerNav, d->mainNav, current);
            });

    connect(d->mainNav,
            &ExNavTreeWidget::pageIndexChanged,
            this,
            [this, d](int pageIndex)
            {
                if (d->stackedWidget)
                    d->stackedWidget->setCurrentIndex(pageIndex);
                emit pageIndexChanged(pageIndex);
            });
    connect(d->footerNav,
            &ExNavTreeWidget::pageIndexChanged,
            this,
            [this, d](int pageIndex)
            {
                if (d->stackedWidget)
                    d->stackedWidget->setCurrentIndex(pageIndex);
                emit pageIndexChanged(pageIndex);
            });
}

ExWinUINavigationView::~ExWinUINavigationView() = default;

ExNavTreeWidget *ExWinUINavigationView::mainNavView() const
{
    Q_D(const ExWinUINavigationView);
    return d->mainNav;
}

ExNavTreeWidget *ExWinUINavigationView::footerNavView() const
{
    Q_D(const ExWinUINavigationView);
    return d->footerNav;
}

void ExWinUINavigationView::setStackedWidget(QStackedWidget *stack)
{
    Q_D(ExWinUINavigationView);
    d->stackedWidget = stack;
    if (d->stackedWidget)
        d->stackedWidget->setCurrentIndex(selectedPageIndex());
}

QStackedWidget *ExWinUINavigationView::stackedWidget() const
{
    Q_D(const ExWinUINavigationView);
    return d->stackedWidget;
}

bool ExWinUINavigationView::setSelectedPageIndex(int pageIndex)
{
    Q_D(ExWinUINavigationView);
    const auto matchByPage = [pageIndex](QTreeWidgetItem *item)
    {
        return item && item->data(0, NavigationPageRole).toInt() == pageIndex;
    };

    for (QTreeWidgetItemIterator it(d->mainNav); *it; ++it)
    {
        if (matchByPage(*it))
        {
            d->mainNav->setCurrentItem(*it);
            return true;
        }
    }

    for (QTreeWidgetItemIterator it(d->footerNav); *it; ++it)
    {
        if (matchByPage(*it))
        {
            d->footerNav->setCurrentItem(*it);
            return true;
        }
    }

    return false;
}

int ExWinUINavigationView::selectedPageIndex() const
{
    Q_D(const ExWinUINavigationView);
    const auto itemPage = [](QTreeWidgetItem *item) -> int
    {
        if (!item)
            return -1;
        const QVariant pageData = item->data(0, NavigationPageRole);
        return pageData.isValid() ? pageData.toInt() : -1;
    };

    const int mainPage = itemPage(d->mainNav ? d->mainNav->currentItem() : nullptr);
    if (mainPage >= 0)
        return mainPage;
    return itemPage(d->footerNav ? d->footerNav->currentItem() : nullptr);
}

void ExWinUINavigationView::setNavigationExpanded(bool expanded, bool animated)
{
    Q_D(ExWinUINavigationView);
    if (d->mainNav)
        d->mainNav->setNavigationExpanded(expanded, animated);
    if (d->footerNav)
        d->footerNav->setNavigationExpanded(expanded, animated);
}

bool ExWinUINavigationView::navigationExpanded() const
{
    Q_D(const ExWinUINavigationView);
    return d->mainNav && d->mainNav->navigationExpanded();
}

QTreeWidgetItem *ExWinUINavigationView::addNavigationItem(const QString &text,
                                                          int pageIndex,
                                                          const QString &iconCode)
{
    return addMainNavigationItem(text, pageIndex, iconCode);
}

QTreeWidgetItem *ExWinUINavigationView::addMainNavigationItem(const QString &text,
                                                              int pageIndex,
                                                              const QString &iconCode)
{
    Q_D(ExWinUINavigationView);
    if (!d->mainNav)
        return nullptr;
    QTreeWidgetItem *item = d->mainNav->addNavigationItem(text, pageIndex, iconCode);
    if (item && !d->mainNav->currentItem() && d->mainNav->topLevelItemCount() == 1)
        d->mainNav->setCurrentItem(item);
    return item;
}

QTreeWidgetItem *ExWinUINavigationView::addFooterNavigationItem(const QString &text,
                                                                int pageIndex,
                                                                const QString &iconCode)
{
    Q_D(ExWinUINavigationView);
    return d->footerNav ? d->footerNav->addNavigationItem(text, pageIndex, iconCode) : nullptr;
}

void ExWinUINavigationView::setCurrentMainItem(QTreeWidgetItem *item)
{
    Q_D(ExWinUINavigationView);
    if (d->mainNav)
        d->mainNav->setCurrentItem(item);
}

void ExWinUINavigationView::clearFooterSelection()
{
    Q_D(ExWinUINavigationView);
    if (!d->footerNav)
        return;

    d->footerNav->clearSelection();
    d->footerNav->setCurrentIndex(QModelIndex());
}

void ExWinUINavigationView::handleCrossViewSelection(ExNavTreeWidget *activatedNav,
                                                     ExNavTreeWidget *peerNav,
                                                     QTreeWidgetItem *current)
{
    if (!current || !activatedNav || !peerNav || !peerNav->selectionModel() ||
        !peerNav->selectionModel()->hasSelection())
    {
        return;
    }

    const bool activatedIsFooter = (activatedNav == footerNavView());
    peerNav->setProperty("navigationDirection",
                         activatedIsFooter ? QStringLiteral("down") : QStringLiteral("up"));
    activatedNav->setProperty("navigationDirection",
                              activatedIsFooter ? QStringLiteral("up") : QStringLiteral("down"));

    peerNav->clearSelection();
    peerNav->setCurrentIndex(QModelIndex());
    if (peerNav->viewport())
        peerNav->viewport()->update();
}
