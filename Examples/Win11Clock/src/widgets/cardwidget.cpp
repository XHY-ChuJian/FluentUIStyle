#include "widgets/cardwidget.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOption>

CardWidget::CardWidget(QWidget* parent)
    : QWidget(parent)
{
    setProperty("isCard", QVariant(true));
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void CardWidget::paintEvent(QPaintEvent*)
{
    QStyleOption option;
    option.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}
