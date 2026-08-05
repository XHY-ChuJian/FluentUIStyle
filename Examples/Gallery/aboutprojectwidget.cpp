#include "aboutprojectwidget.h"

#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QVBoxLayout>

AboutProjectWidget::AboutProjectWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(9, 9, 9, 9);
    layout->setSpacing(0);

    QFrame *card = new QFrame(this);
    card->setObjectName(QStringLiteral("aboutCard"));

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(9, 9, 9, 9);
    cardLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel(QStringLiteral("关于项目"), card);
    titleLabel->setObjectName(QStringLiteral("aboutTitle"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *subtitleLabel = new QLabel(QStringLiteral("FluentUI3Style · Qt FluentUI (WinUI3) 风格实现"), card);
    subtitleLabel->setObjectName(QStringLiteral("aboutSubtitle"));
    QPalette subtitlePalette = subtitleLabel->palette();
    subtitlePalette.setColor(QPalette::WindowText, subtitlePalette.color(QPalette::Mid));
    subtitleLabel->setPalette(subtitlePalette);

    QLabel *contentLabel = new QLabel(card);
    contentLabel->setObjectName(QStringLiteral("aboutProjectContent"));
    contentLabel->setWordWrap(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QFont contentFont = contentLabel->font();
    contentFont.setPointSize(11);
    contentLabel->setFont(contentFont);
    contentLabel->setTextFormat(Qt::RichText);
    contentLabel->setText(QStringLiteral(
        "<p>本项目定位为样式库，目标是将 Qt 现有控件呈现为 FluentUI（WinUI3）风格。</p>"
        "<p>由于 Qt 组件边界限制，部分 FluentUI 控件无法完全复刻；但会尽量基于现有控件，通过 Style 中的定制逻辑实现接近 FluentUI 的交互与视觉效果，例如：</p>"
        "<ul>"
        "<li>SwitchButton</li>"
        "<li>TabBar 实现 \"Pivot\" 和 \"Segmented\" 控件</li>"
        "</ul>"
        "<p>为了让 \"Gallery\" 展示更完整，会在 ExWidgets 下实现一些组件；这些组件的样式仍由 Style 统一绘制。"
        "后续可能会增加其他控件，但都会保证这些组件能独立于 Style 之外运行。</p>"
        "<p>若想修改控件样式，只能通过以往使用 QSS 的方式，那样势必会使控件的 QStyle 样式消失。"
        "如需深度定制，建议像本项目一样重写对应的 QStyle 逻辑，但这属于深度定制的功能了，对于本项目不是很合适。</p>"
        "<p>所以如果需要统一样式，或者在统一样式下做一些小改动，推荐本项目。"
        "最后，本项目是为了在现有项目中，或者希望简单集成 FluentUI 样式时使用而实现。</p>"));

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addWidget(contentLabel, 1);
    layout->addWidget(card, 1);
}

