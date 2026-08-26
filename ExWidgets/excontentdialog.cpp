#include "excontentdialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
// ---- WinUI 3 ContentDialog 设计常量 ----
// 参考:
// https://learn.microsoft.com/en-us/windows/apps/design/controls/dialogs-and-flyouts/dialogs
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

bool isDarkMode() {
  return QApplication::palette().window().color().lightness() < 128;
}

QColor dialogBorderColor(bool dark) {
  return dark ? QColor(255, 255, 255, 20) : QColor(0, 0, 0, 15);
}
} // namespace

class ExContentDialogPrivate {
public:
  ExContentDialogPrivate(ExContentDialog *q) : q_ptr(q) {}

  ExContentDialog *q_ptr;

  QWidget *m_cardWidget{nullptr};
  QLabel *m_titleLabel{nullptr};
  QLabel *m_contentLabel{nullptr};
  QWidget *m_contentWidget{nullptr};
  QVBoxLayout *m_contentArea{nullptr};
  QHBoxLayout *m_buttonArea{nullptr};
  QPushButton *m_primaryButton{nullptr};
  QPushButton *m_secondaryButton{nullptr};
  QPushButton *m_closeButton{nullptr};

  ExContentDialog::ContentDialogButton m_defaultButton{
      ExContentDialog::NoneButton};
  ExContentDialog::ContentDialogResult m_result{ExContentDialog::None};

  void buildUI();
  void updateButtonVisibility();
  void applyDefaultButtonStyle();
};

void ExContentDialogPrivate::buildUI() {
  ExContentDialog *q = q_ptr;

  auto *outerLayout = new QVBoxLayout(q);
  outerLayout->setContentsMargins(kShadowMargin, kShadowMargin, kShadowMargin,
                                  kShadowMargin);
  outerLayout->setSpacing(0);

  m_cardWidget = new QWidget(q);
  m_cardWidget->setObjectName(QStringLiteral("ExContentDialogCard"));
  outerLayout->addWidget(m_cardWidget);

  auto *innerLayout = new QVBoxLayout(m_cardWidget);
  innerLayout->setContentsMargins(kPadding, kPadding, kPadding, 0);
  innerLayout->setSpacing(0);

  m_titleLabel = new QLabel(m_cardWidget);
  m_titleLabel->setObjectName(QStringLiteral("ExContentDialogTitle"));
  m_titleLabel->setWordWrap(true);
  {
    QFont f = m_titleLabel->font();
    f.setPixelSize(kTitleFontPx);
    f.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(f);
  }
  innerLayout->addWidget(m_titleLabel);
  innerLayout->addSpacing(kTitleContentGap);

  m_contentArea = new QVBoxLayout;
  m_contentArea->setContentsMargins(0, 0, 0, 0);
  m_contentArea->setSpacing(0);

  m_contentLabel = new QLabel(m_cardWidget);
  m_contentLabel->setObjectName(QStringLiteral("ExContentDialogContent"));
  m_contentLabel->setWordWrap(true);
  {
    QFont f = m_contentLabel->font();
    f.setPixelSize(kBodyFontPx);
    m_contentLabel->setFont(f);
  }
  m_contentArea->addWidget(m_contentLabel);
  innerLayout->addLayout(m_contentArea);
  innerLayout->addSpacing(kContentButtonGap);

  m_buttonArea = new QHBoxLayout;
  m_buttonArea->setContentsMargins(0, kPadding, 0, kPadding);
  m_buttonArea->setSpacing(kButtonSpacing);
  m_buttonArea->addStretch();

  m_closeButton = new QPushButton(m_cardWidget);
  m_closeButton->setObjectName(QStringLiteral("ExContentDialogClose"));
  m_closeButton->setMinimumWidth(kButtonMinWidth);
  m_closeButton->setVisible(false);

  m_secondaryButton = new QPushButton(m_cardWidget);
  m_secondaryButton->setObjectName(QStringLiteral("ExContentDialogSecondary"));
  m_secondaryButton->setMinimumWidth(kButtonMinWidth);
  m_secondaryButton->setVisible(false);

  m_primaryButton = new QPushButton(m_cardWidget);
  m_primaryButton->setObjectName(QStringLiteral("ExContentDialogPrimary"));
  m_primaryButton->setMinimumWidth(kButtonMinWidth);
  m_primaryButton->setVisible(false);

  m_buttonArea->addWidget(m_closeButton, 0, Qt::AlignVCenter);
  m_buttonArea->addWidget(m_secondaryButton, 0, Qt::AlignVCenter);
  m_buttonArea->addWidget(m_primaryButton, 0, Qt::AlignVCenter);
  m_buttonArea->addStretch();
  innerLayout->addLayout(m_buttonArea);

  q->setMinimumWidth(kDialogMinWidth + 2 * kShadowMargin);
  q->setMaximumWidth(kDialogMaxWidth + 2 * kShadowMargin);

  QObject::connect(m_primaryButton, &QPushButton::clicked, q, [this, q]() {
    m_result = ExContentDialog::Primary;
    emit q->primaryButtonClicked();
    q->accept();
  });
  QObject::connect(m_secondaryButton, &QPushButton::clicked, q, [this, q]() {
    m_result = ExContentDialog::Secondary;
    emit q->secondaryButtonClicked();
    q->accept();
  });
  QObject::connect(m_closeButton, &QPushButton::clicked, q, [this, q]() {
    m_result = ExContentDialog::None;
    emit q->closeButtonClicked();
    q->reject();
  });
}

void ExContentDialogPrivate::updateButtonVisibility() {
  m_primaryButton->setVisible(!m_primaryButton->text().isEmpty());
  m_secondaryButton->setVisible(!m_secondaryButton->text().isEmpty());
  m_closeButton->setVisible(!m_closeButton->text().isEmpty());
}

void ExContentDialogPrivate::applyDefaultButtonStyle() {
  // 先重置所有按钮状态
  for (QPushButton *btn : {m_primaryButton, m_secondaryButton, m_closeButton}) {
    btn->setDefault(false);
    btn->setProperty("accent", false);
  }

  QPushButton *target = nullptr;
  switch (m_defaultButton) {
  case ExContentDialog::PrimaryBtn:
    target = m_primaryButton;
    break;
  case ExContentDialog::SecondaryBtn:
    target = m_secondaryButton;
    break;
  case ExContentDialog::CloseBtn:
    target = m_closeButton;
    break;
  default:
    break;
  }

  if (target) {
    target->setProperty("accent", true);
    target->setFocus();
  }
}

ExContentDialog::ExContentDialog(QWidget *parent)
    : QDialog(parent), d_ptr(new ExContentDialogPrivate(this)) {
  setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);
  d_ptr->buildUI();
}

ExContentDialog::~ExContentDialog() { delete d_ptr; }

QString ExContentDialog::title() const {
  Q_D(const ExContentDialog);
  return d->m_titleLabel->text();
}

void ExContentDialog::setTitle(const QString &title) {
  Q_D(ExContentDialog);
  d->m_titleLabel->setText(title);
  d->m_titleLabel->setVisible(!title.isEmpty());
}

QString ExContentDialog::content() const {
  Q_D(const ExContentDialog);
  return d->m_contentLabel->text();
}

void ExContentDialog::setContent(const QString &text) {
  Q_D(ExContentDialog);
  d->m_contentLabel->setText(text);
  d->m_contentLabel->setVisible(!text.isEmpty());
  if (d->m_contentWidget)
    d->m_contentWidget->setVisible(false);
}

void ExContentDialog::setContentWidget(QWidget *widget) {
  Q_D(ExContentDialog);
  if (d->m_contentWidget)
    d->m_contentArea->removeWidget(d->m_contentWidget);
  d->m_contentWidget = widget;
  if (widget) {
    d->m_contentLabel->setVisible(false);
    d->m_contentArea->addWidget(widget);
    widget->setVisible(true);
  }
}

QString ExContentDialog::primaryButtonText() const {
  Q_D(const ExContentDialog);
  return d->m_primaryButton->text();
}

void ExContentDialog::setPrimaryButtonText(const QString &text) {
  Q_D(ExContentDialog);
  d->m_primaryButton->setText(text);
  d->updateButtonVisibility();
}

QString ExContentDialog::secondaryButtonText() const {
  Q_D(const ExContentDialog);
  return d->m_secondaryButton->text();
}

void ExContentDialog::setSecondaryButtonText(const QString &text) {
  Q_D(ExContentDialog);
  d->m_secondaryButton->setText(text);
  d->updateButtonVisibility();
}

QString ExContentDialog::closeButtonText() const {
  Q_D(const ExContentDialog);
  return d->m_closeButton->text();
}

void ExContentDialog::setCloseButtonText(const QString &text) {
  Q_D(ExContentDialog);
  d->m_closeButton->setText(text);
  d->updateButtonVisibility();
}

void ExContentDialog::setIsPrimaryButtonEnabled(bool enabled) {
  Q_D(ExContentDialog);
  d->m_primaryButton->setEnabled(enabled);
}

void ExContentDialog::setIsSecondaryButtonEnabled(bool enabled) {
  Q_D(ExContentDialog);
  d->m_secondaryButton->setEnabled(enabled);
}

ExContentDialog::ContentDialogButton ExContentDialog::defaultButton() const {
  Q_D(const ExContentDialog);
  return d->m_defaultButton;
}

void ExContentDialog::setDefaultButton(ContentDialogButton button) {
  Q_D(ExContentDialog);
  d->m_defaultButton = button;
  d->applyDefaultButtonStyle();
}

ExContentDialog::ContentDialogResult ExContentDialog::result() const {
  Q_D(const ExContentDialog);
  return d->m_result;
}

ExContentDialog::ContentDialogResult ExContentDialog::showDialog() {
  Q_D(ExContentDialog);
  d->m_result = None;
  d->applyDefaultButtonStyle();
  adjustSize();
  exec();
  return d->m_result;
}

void ExContentDialog::paintEvent(QPaintEvent * /*event*/) {
  Q_D(ExContentDialog);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  const QRectF cardRect = d->m_cardWidget->geometry();
  const bool dark = isDarkMode();

  painter.setPen(Qt::NoPen);
  constexpr int layers = 4;
  for (int i = layers; i > 0; --i) {
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

  const QPoint btnBarTopLeft =
      d->m_buttonArea->geometry().topLeft() + d->m_cardWidget->pos();
  const int splitY = btnBarTopLeft.y();

  QRectF topArea = cardRect;
  topArea.setBottom(splitY);
  painter.fillRect(topArea, dark ? palette().base() : Qt::white);

  QRectF bottomArea = cardRect;
  bottomArea.setTop(splitY);
  painter.fillRect(bottomArea, palette().window());

  painter.setClipping(false);
  painter.setPen(QPen(dialogBorderColor(dark), 1.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(cardRect.adjusted(0.5, 0.5, -0.5, -0.5),
                          kCornerRadius, kCornerRadius);
}

void ExContentDialog::keyPressEvent(QKeyEvent *event) {
  Q_D(ExContentDialog);
  if (event->key() == Qt::Key_Escape) {
    d->m_result = None;
    emit closeButtonClicked();
    reject();
    return;
  }
  QDialog::keyPressEvent(event);
}
