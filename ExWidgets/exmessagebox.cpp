#include "exmessagebox.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
// ---- WinUI 3 ContentDialog 设计常量 ----
// 参考: https://learn.microsoft.com/en-us/windows/apps/design/controls/dialogs-and-flyouts/dialogs
constexpr int kCornerRadius = 8;
constexpr int kPadding = 16;
constexpr int kTitleContentGap = 12;
constexpr int kContentButtonGap = 16;
constexpr int kButtonSpacing = 8;
constexpr int kButtonHeight = 32;
constexpr int kButtonMinWidth = 120;
constexpr int kTitleFontPx = 20;
constexpr int kBodyFontPx = 14;
constexpr int kInformativeFontPx = 13;
constexpr int kShadowMargin = 8;

constexpr int kDialogMinWidth = 320;
constexpr int kDialogPreferWidth = 480;
constexpr int kDialogMaxWidth = 548;

bool isDarkMode()
{
    return QApplication::palette().window().color().lightness() < 128;
}

QColor dialogBorderColor(bool dark)
{
    return dark ? QColor(255, 255, 255, 20) : QColor(0, 0, 0, 15);
}
} // namespace

static void setFontPerferNoHinting(QWidget *widget, bool fontSizeIncrease = false)
{
    if (!widget)
        return;
    QFont f = widget->font();
    f.setHintingPreference(QFont::HintingPreference::PreferNoHinting);

    if (fontSizeIncrease)
    {
        auto size = f.pointSize();
        f.setPointSize(size + 1);
    }
    widget->setFont(f);
}

class ExMessageBoxPrivate
{
public:
    ExMessageBoxPrivate(ExMessageBox *q) : q_ptr(q) {}

    ExMessageBox *q_ptr;

    QWidget *m_cardWidget{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QLayout *m_buttonArea{nullptr};
    QWidget *m_customContentWidget{nullptr};
    bool m_centerButtons{true};

    void resetLayout();
};

void ExMessageBoxPrivate::resetLayout()
{
    ExMessageBox *q = q_ptr;

    auto *iconLabel = q->findChild<QLabel *>(QStringLiteral("qt_msgboxex_icon_label"));
    auto *textLabel = q->findChild<QLabel *>(QStringLiteral("qt_msgbox_label"));
    auto *infoLabel = q->findChild<QLabel *>(QStringLiteral("qt_msgbox_informativelabel"));
    auto *buttonBox = q->findChild<QDialogButtonBox *>(QStringLiteral("qt_msgbox_buttonbox"));

    if (!textLabel || !buttonBox)
        return;

    if (m_cardWidget)
    {
        return;
    }

    delete q->layout();

    auto *outerLayout = new QVBoxLayout();
    outerLayout->setObjectName("OuterLayout");
    outerLayout->setContentsMargins(kShadowMargin, kShadowMargin,
                                    kShadowMargin, kShadowMargin);
    outerLayout->setSpacing(0);
    q->setLayout(outerLayout);

    m_cardWidget = new QWidget(q);
    m_cardWidget->setObjectName(QStringLiteral("FluentMessageBoxCard"));
    outerLayout->addWidget(m_cardWidget);

    auto *innerLayout = new QVBoxLayout(m_cardWidget);
    innerLayout->setObjectName("CardInnerLayout");
    innerLayout->setContentsMargins(kPadding, kPadding, kPadding, 0);
    innerLayout->setSpacing(0);

    if (q->windowTitle().isEmpty())
    {
        q->setWindowTitle(QApplication::applicationName());
    }
    auto *titleLabel = new QLabel(q->windowTitle(), m_cardWidget);
    titleLabel->setObjectName(QStringLiteral("FluentMessageBoxTitle"));
    titleLabel->setWordWrap(true);
    {
        QFont f = titleLabel->font();
        f.setPixelSize(kTitleFontPx);
        f.setWeight(QFont::DemiBold);
        titleLabel->setFont(f);
    }
    innerLayout->addWidget(titleLabel);
    innerLayout->addSpacing(kTitleContentGap);

    {
        QFont f = textLabel->font();
        f.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
        f.setPixelSize(kBodyFontPx);
        textLabel->setFont(f);
    }
    textLabel->setWordWrap(true);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const bool hasIcon = iconLabel && !iconLabel->pixmap().isNull();
#else
    const bool hasIcon = iconLabel && !iconLabel->pixmap()->isNull();
#endif
    if (hasIcon)
    {
        auto *contentRow = new QHBoxLayout;
        contentRow->setSpacing(12);
        contentRow->addWidget(iconLabel, 0, Qt::AlignVCenter);
        contentRow->addWidget(textLabel, 1);
        innerLayout->addLayout(contentRow);
    }
    else
    {
        innerLayout->addWidget(textLabel);
        if (iconLabel)
            iconLabel->hide();
    }
    iconLabel->setParent(m_cardWidget);
    textLabel->setParent(m_cardWidget);

    if (infoLabel)
    {
        QFont f = infoLabel->font();
        f.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
        f.setPixelSize(kInformativeFontPx);
        infoLabel->setFont(f);
        infoLabel->setContentsMargins(0, 7, 0, 0);
        innerLayout->addWidget(infoLabel);
    }

    if (auto *checkBox = q->findChild<QCheckBox *>())
    {
        setFontPerferNoHinting(checkBox);
        checkBox->setContentsMargins(0, 7, 0, 0);
        checkBox->setParent(m_cardWidget);
        innerLayout->addWidget(checkBox);
    }

    if (m_customContentWidget)
    {
        m_customContentWidget->setParent(m_cardWidget);
        innerLayout->addWidget(m_customContentWidget);
    }

    innerLayout->addSpacing(kContentButtonGap);

    auto *buttonArea = new QHBoxLayout(m_cardWidget);
    buttonArea->setObjectName("ButtonAreaLayout");
    int btnMarginV = 12;

    if (m_centerButtons)
        buttonArea->addStretch();

    buttonArea->setSpacing(kButtonSpacing);
    buttonArea->setContentsMargins(0, btnMarginV, 0, btnMarginV);

    buttonBox->setParent(m_cardWidget);

    for (QAbstractButton *ab : buttonBox->buttons())
    {
        if (auto *pb = qobject_cast<QPushButton *>(ab))
        {
            setFontPerferNoHinting(pb, true);
            pb->setDefault(false);
            pb->setMinimumWidth(kButtonMinWidth);
        }
    }

    // QDialogButtonBox 内部限制了按钮布局，导致实际不居中，把按钮取出来后，强制布局
    if (buttonBox->layout())
    {
        QLayout *boxLayout = buttonBox->layout();
        while (QLayoutItem *item = boxLayout->takeAt(0))
        {
            if (QWidget *w = item->widget())
            {
                buttonArea->addWidget(w, 0, Qt::AlignVCenter);
            }
            delete item;
        }
    }
    else
    {
        buttonArea->addWidget(buttonBox);
    }

    if (m_centerButtons)
        buttonArea->addStretch();

    buttonBox->hide();

    innerLayout->addLayout(buttonArea);

    m_buttonBox = buttonBox;
    m_buttonArea = buttonArea;

    q->setMinimumWidth(kDialogMinWidth + 2 * kShadowMargin);
    q->setMaximumWidth(kDialogMaxWidth + 2 * kShadowMargin);
}

ExMessageBox::ExMessageBox(QWidget *parent)
    : QMessageBox(parent), d_ptr(new ExMessageBoxPrivate(this))
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground);
}

ExMessageBox::ExMessageBox(Icon icon,
                           const QString &title,
                           const QString &text,
                           StandardButtons buttons,
                           QWidget *parent,
                           Qt::WindowFlags f)
    : QMessageBox(icon, title, text, buttons, parent, f), d_ptr(new ExMessageBoxPrivate(this))
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground);
}

ExMessageBox::~ExMessageBox()
{
    delete d_ptr;
}

void ExMessageBox::setContentWidget(QWidget *widget)
{
    Q_D(ExMessageBox);
    d->m_customContentWidget = widget;
}

void ExMessageBox::setCenterButtons(bool center)
{
    Q_D(ExMessageBox);
    d->m_centerButtons = center;
}

bool ExMessageBox::centerButtons() const
{
    Q_D(const ExMessageBox);
    return d->m_centerButtons;
}

void ExMessageBox::setVisible(bool visible)
{
    Q_D(ExMessageBox);
    if (visible)
    {
        d->resetLayout();
    }
    QMessageBox::setVisible(visible);
}

void ExMessageBox::paintEvent(QPaintEvent *event)
{
    Q_D(ExMessageBox);
    if (!d->m_cardWidget)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect cardRect = this->rect();
    cardRect.adjust(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);

    const bool dark = isDarkMode();

    painter.setPen(Qt::NoPen);
    constexpr int layers = 4;
    for (int i = layers; i > 0; --i)
    {
        const qreal expand = i * 1.5;
        const int alpha = dark ? (6 + (layers - i) * 4) : (3 + (layers - i) * 2);
        QRectF sr = cardRect.adjusted(-expand, -expand + 1, expand, expand + 1);
        QPainterPath sp;
        sp.addRoundedRect(sr, kCornerRadius + expand, kCornerRadius + expand);
        painter.setBrush(QColor(0, 0, 0, alpha));
        painter.drawPath(sp);
    }

    QPainterPath path;
    path.addRoundedRect(cardRect, kCornerRadius, kCornerRadius);
    painter.setClipPath(path);

    QPoint mappedTopLeft = d->m_cardWidget->mapTo(this, d->m_buttonArea->geometry().topLeft());
    const int splitY = mappedTopLeft.y();

    QRect topArea = cardRect;
    topArea.setBottom(splitY);
    painter.fillRect(topArea, dark ? palette().base() : Qt::white);

    QRect bottomArea = cardRect;
    bottomArea.setTop(splitY);
    painter.fillRect(bottomArea, palette().window());

    painter.setClipping(false);
    painter.setPen(QPen(dialogBorderColor(dark), 1.0));
    painter.setBrush(Qt::NoBrush);
    const auto cardRectF = QRectF(cardRect);
    painter.drawRoundedRect(cardRectF.adjusted(0.5, 0.5, -0.5, -0.5),
                            kCornerRadius, kCornerRadius);
}

void ExMessageBox::showEvent(QShowEvent *event)
{
    // 拦截 QMessageBox 的 showEvent，避免其强制触发内部的 updateSize()
    QDialog::showEvent(event);
}

void ExMessageBox::resizeEvent(QResizeEvent *event)
{
    // 拦截 QMessageBox 的 resizeEvent
    QDialog::resizeEvent(event);
}

bool ExMessageBox::event(QEvent *e)
{
    // 拦截 QMessageBox 的 event，避免其在 LayoutRequest 时强制触发 updateSize()
    if (e->type() == QEvent::LayoutRequest)
    {
        return QDialog::event(e);
    }
    return QMessageBox::event(e);
}

QMessageBox::StandardButton ExMessageBox::information(QWidget *parent,
                                                      const QString &title,
                                                      const QString &text,
                                                      StandardButtons buttons,
                                                      StandardButton defaultButton)
{
    ExMessageBox box(Information, title, text, buttons, parent);
    return static_cast<StandardButton>(box.exec());
}

QMessageBox::StandardButton ExMessageBox::warning(QWidget *parent,
                                                  const QString &title,
                                                  const QString &text,
                                                  StandardButtons buttons,
                                                  StandardButton defaultButton)
{
    ExMessageBox box(Warning, title, text, buttons, parent);
    return static_cast<StandardButton>(box.exec());
}

QMessageBox::StandardButton ExMessageBox::critical(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   StandardButtons buttons,
                                                   StandardButton defaultButton)
{
    ExMessageBox box(Critical, title, text, buttons, parent);
    return static_cast<StandardButton>(box.exec());
}

QMessageBox::StandardButton ExMessageBox::question(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   StandardButtons buttons,
                                                   StandardButton defaultButton)
{
    ExMessageBox box(Question, title, text, buttons, parent);
    return static_cast<StandardButton>(box.exec());
}
