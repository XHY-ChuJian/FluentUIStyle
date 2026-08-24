#include "fluenttitlebar.h"

#include "qlineedit.h"

#ifndef WIN32
#include "fluentui3style.h"
#endif

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>

namespace {

constexpr QChar kChromeMinimize(0xE921);
constexpr QChar kChromeMaximize(0xE922);
constexpr QChar kChromeRestore(0xE923);
constexpr QChar kChromeClose(0xE8BB);
constexpr QChar kChromeTheme(0xE706);
constexpr QChar kChromePin(0xE718);
constexpr QChar kChromePinned(0xE77A);
constexpr QChar kChromeSearch(0xE721);

const QList<QColor> defaultAccentColors = {
    QColor(),                          // 默认强调色 (Windows 11 Fluent Blue)
    QColor(QStringLiteral("#5B5FC7")), // Microsoft Teams 品牌紫
    QColor(QStringLiteral("#107C41"))  // Microsoft Excel / Fluent 品牌绿
};

QFont captionIconFont(int pixelSize = 11) {
  QFont font(QStringLiteral("Segoe Fluent Icons"));
  font.setPixelSize(pixelSize);
  font.setStyleStrategy(QFont::PreferAntialias);
  return font;
}

QToolButton *createCaptionButton(QWidget *parent, const QString &objectName,
                                 const QChar &glyph, int width = 46,
                                 int pixelSize = 11) {
  auto *button = new QToolButton(parent);
  button->setObjectName(objectName);
  button->setAutoRaise(true);
  button->setToolButtonStyle(Qt::ToolButtonTextOnly);
  button->setFont(captionIconFont(pixelSize));
  button->setText(QString(glyph));
  button->setFixedSize(width, 40);
  return button;
}

QIcon searchIcon(bool darkTheme) {
  constexpr int iconSize = 32;
  QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
  iconFont.setPixelSize(iconSize);

  QPixmap pixmap(iconSize, iconSize);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
  painter.setFont(iconFont);
  painter.setPen(darkTheme ? Qt::white : Qt::black);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, QString(kChromeSearch));

  return QIcon(pixmap);
}

} // namespace

FluentAccentColorButton::FluentAccentColorButton(QWidget *parent)
    : QToolButton(parent) {
  setCheckable(true);
  setAutoRaise(true);
  setFixedSize(20, 20);
  setCursor(Qt::PointingHandCursor);
  setStyleSheet(
      QStringLiteral("QToolButton { background: transparent; border: none; }"));
}

FluentAccentColorButton::FluentAccentColorButton(const QColor &color,
                                                 QWidget *parent)
    : QToolButton(parent), m_color(color) {
  setCheckable(true);
  setAutoRaise(true);
  setFixedSize(20, 20);
  setCursor(Qt::PointingHandCursor);
  setStyleSheet(
      QStringLiteral("QToolButton { background: transparent; border: none; }"));
}

QColor FluentAccentColorButton::color() const { return m_color; }

void FluentAccentColorButton::setColor(const QColor &color) {
  m_color = color;
  update();
}

bool FluentAccentColorButton::isDefaultColor() const { return m_isDefault; }

void FluentAccentColorButton::setIsDefaultColor(bool isDefault) {
  m_isDefault = isDefault;
  update();
}

void FluentAccentColorButton::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QPointF center = QRectF(rect()).center();
  const qreal minDimension = qMin(width(), height());
  const qreal r_outer = (minDimension / 2.0) - 2.0;

  QColor baseColor =
      m_color.isValid() ? m_color : QColor(QStringLiteral("#0078D4"));
  QColor drawColor = baseColor;
  if (isDown()) {
    drawColor = drawColor.darker(115);
  } else if (underMouse()) {
    drawColor = drawColor.lighter(112);
  }

  const bool isDark = qApp->property("_q_colorscheme").toInt() == 1;

  if (isChecked()) {
    // 选中态：两个同心圆 (Outer Ring + Inner Solid Dot)
    QPen ringPen(drawColor, 2.0);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, r_outer - 0.5, r_outer - 0.5);

    const qreal r_inner = qMax(3.2, r_outer - 2.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(drawColor);
    painter.drawEllipse(center, r_inner, r_inner);
  } else {
    // 未选中态：外围带细腻微透明细圈轮廓，内部实心填充圆
    const QColor outlineColor =
        isDark ? QColor(255, 255, 255, underMouse() ? 120 : 60)
               : QColor(0, 0, 0, underMouse() ? 90 : 40);

    painter.setPen(QPen(outlineColor, 1.0));
    painter.setBrush(drawColor);
    const qreal r_fill = underMouse() ? (r_outer - 0.5) : (r_outer - 1.2);
    painter.drawEllipse(center, r_fill, r_fill);
  }
}

FluentTitleBar::FluentTitleBar(QMainWindow *window)
    : QWidget(window), m_window(window) {
  setObjectName(QStringLiteral("fluent-title-bar"));
  setFixedHeight(40);
  setAttribute(Qt::WA_StyledBackground, true);

  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(16, 16);
  m_iconLabel->setScaledContents(true);

  m_titleLabel = new QLabel(this);
  m_titleLabel->setObjectName(QStringLiteral("fluent-title-label"));

  m_accentButtonGroup = new QButtonGroup(this);
  m_accentButtonGroup->setExclusive(true);

  m_accentColors = defaultAccentColors;
  for (int i = 0; i < m_accentColors.size(); ++i) {
    auto *btn = new FluentAccentColorButton(this);
    btn->setObjectName(QStringLiteral("win_caption_accent_%1").arg(i));
    btn->setColor(m_accentColors[i]);
    btn->setIsDefaultColor(!m_accentColors[i].isValid());
    if (!m_accentColors[i].isValid()) {
      btn->setToolTip(QObject::tr("默认强调色"));
    } else {
      btn->setToolTip(QObject::tr("强调色: %1").arg(m_accentColors[i].name()));
    }
    m_accentButtons.append(btn);
    m_accentButtonGroup->addButton(btn, i);
  }

#if QT_VERSION <= QT_VERSION_CHECK(5, 15, 2)
  connect(
      m_accentButtonGroup,
      static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
      this, &FluentTitleBar::onAccentButtonClicked);
#else
  connect(m_accentButtonGroup, &QButtonGroup::idClicked, this,
          &FluentTitleBar::onAccentButtonClicked);
#endif

  m_themeButton = createCaptionButton(this, QStringLiteral("win_caption_theme"),
                                      kChromeTheme, 40, 16);
  m_pinButton = createCaptionButton(this, QStringLiteral("win_caption_pin"),
                                    kChromePin, 40, 16);
  m_pinButton->setCheckable(true);

  m_minButton = createCaptionButton(
      this, QStringLiteral("win_caption_minimize"), kChromeMinimize);
  m_maxButton = createCaptionButton(
      this, QStringLiteral("win_caption_maximize"), kChromeMaximize);
  m_maxButton->setCheckable(true);
  m_closeButton = createCaptionButton(this, QStringLiteral("win_caption_close"),
                                      kChromeClose);

  m_searchLineEdit = new QLineEdit(this);
  m_searchLineEdit->setMinimumWidth(300);
  m_searchLineEdit->setPlaceholderText(tr("搜索..."));
  m_searchLineEdit->setClearButtonEnabled(true);

#if !(defined(__MINGW32__) || defined(Q_OS_LINUX))
  m_searchLineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
  // 默认是popup, 会触发系统阴影
  connect(m_searchLineEdit, &QLineEdit::customContextMenuRequested, this,
          [this](const QPoint &pos) {
            if (QMenu *menu = m_searchLineEdit->createStandardContextMenu()) {
              menu->setAttribute(Qt::WA_DeleteOnClose);
              menu->exec(m_searchLineEdit->mapToGlobal(pos));
            }
          });
  // ========================
#endif

  m_searchAction = m_searchLineEdit->addAction(searchIcon(m_themeDark),
                                               QLineEdit::TrailingPosition);

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 0, 0, 0);
  layout->setSpacing(8);
  layout->addWidget(m_iconLabel);
  layout->addWidget(m_titleLabel);
  layout->addStretch();

  layout->addWidget(m_searchLineEdit, 0, Qt::AlignCenter);
  layout->addStretch();

  for (auto *btn : m_accentButtons) {
    layout->addWidget(btn, 0, Qt::AlignVCenter);
  }
  layout->addSpacing(4);

  layout->addWidget(m_themeButton);
  layout->addWidget(m_pinButton);
  layout->addWidget(m_minButton);
  layout->addWidget(m_maxButton);
  layout->addWidget(m_closeButton);

  connect(m_minButton, &QToolButton::clicked, m_window,
          &QWidget::showMinimized);
  connect(m_maxButton, &QToolButton::clicked, m_window, [this]() {
    if (m_window->isMaximized()) {
      m_window->showNormal();
    } else {
      m_window->showMaximized();
    }
  });
  connect(m_closeButton, &QToolButton::clicked, m_window, &QWidget::close);
  connect(m_pinButton, &QToolButton::toggled, this,
          [this]() { updatePinButton(); });

  updateTitle();
  updateIcon();
  updateMaxButton();
  updateThemeButton();
  updatePinButton();
  updateAccentButtons();
  m_window->installEventFilter(this);
}

QList<QToolButton *> FluentTitleBar::accentButtons() const {
  return m_accentButtons;
}

QColor FluentTitleBar::currentAccentColor() const {
  if (m_currentAccentIndex >= 0 &&
      m_currentAccentIndex < m_accentColors.size()) {
    return m_accentColors[m_currentAccentIndex];
  }
  return QColor();
}

void FluentTitleBar::setAccentColor(const QColor &color) {
  int matchIndex = -1;
  for (int i = 0; i < m_accentColors.size(); ++i) {
    if (m_accentColors[i] == color) {
      matchIndex = i;
      break;
    }
  }

  m_currentAccentIndex = matchIndex;
  updateAccentButtons();
}

void FluentTitleBar::setAccentColors(const QList<QColor> &colors) {
  if (colors.isEmpty()) {
    return;
  }

  m_accentColors = colors;
  for (int i = 0; i < m_accentButtons.size(); ++i) {
    auto *btn = static_cast<FluentAccentColorButton *>(m_accentButtons[i]);
    if (i < m_accentColors.size()) {
      btn->setVisible(true);
      btn->setColor(m_accentColors[i]);
      btn->setToolTip(QObject::tr("强调色: %1").arg(m_accentColors[i].name()));
    } else {
      btn->setVisible(false);
    }
  }
  updateAccentButtons();
}

QToolButton *FluentTitleBar::themeButton() const { return m_themeButton; }

QToolButton *FluentTitleBar::pinButton() const { return m_pinButton; }

QToolButton *FluentTitleBar::minButton() const { return m_minButton; }

QToolButton *FluentTitleBar::maxButton() const { return m_maxButton; }

QToolButton *FluentTitleBar::closeButton() const { return m_closeButton; }

QLineEdit *FluentTitleBar::searchLineEdit() const { return m_searchLineEdit; }

void FluentTitleBar::setThemeDark(bool dark) {
  if (m_themeDark == dark) {
    return;
  }

  m_themeDark = dark;
  updateThemeButton();
  updateAccentButtons();
  if (m_searchAction) {
    m_searchAction->setIcon(searchIcon(m_themeDark));
  }
}

void FluentTitleBar::setPinned(bool pinned) {
  m_pinned = pinned;
  if (m_pinButton->isChecked() != pinned) {
    m_pinButton->setChecked(pinned);
  } else {
    updatePinButton();
  }
}

bool FluentTitleBar::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_window) {
    return QWidget::eventFilter(watched, event);
  }

  switch (event->type()) {
  case QEvent::WindowIconChange:
    updateIcon();
    break;
  case QEvent::WindowTitleChange:
    updateTitle();
    break;
  case QEvent::WindowStateChange:
    updateMaxButton();
    break;
  default:
    break;
  }

  return QWidget::eventFilter(watched, event);
}

void FluentTitleBar::updateTitle() {
  m_titleLabel->setText(m_window->windowTitle());
}

void FluentTitleBar::updateIcon() {
  QIcon icon = m_window->windowIcon();
  if (icon.isNull()) {
    icon = QApplication::windowIcon();
  }
  if (icon.isNull()) {
    icon = QIcon(QStringLiteral(":/appicon.ico"));
  }
  if (icon.isNull()) {
    m_iconLabel->clear();
    return;
  }

  m_iconLabel->setPixmap(icon.pixmap(16, 16));
}

void FluentTitleBar::updateMaxButton() {
  const bool maximized = m_window->isMaximized();
  m_maxButton->setChecked(maximized);
  m_maxButton->setText(maximized ? QString(kChromeRestore)
                                 : QString(kChromeMaximize));
}

void FluentTitleBar::updateThemeButton() {
  m_themeButton->setText(QString(kChromeTheme));
  m_themeButton->setToolTip(m_themeDark ? QObject::tr("切换到浅色主题")
                                        : QObject::tr("切换到暗色主题"));
}

void FluentTitleBar::updatePinButton() {
  const bool pinned = m_pinButton->isChecked();
  m_pinned = pinned;
  m_pinButton->setText(pinned ? QString(kChromePinned) : QString(kChromePin));
  m_pinButton->setToolTip(pinned ? QObject::tr("取消置顶")
                                 : QObject::tr("置顶窗口"));
  m_pinButton->update();
}

void FluentTitleBar::updateAccentButtons() {
  for (int i = 0; i < m_accentButtons.size(); ++i) {
    auto *btn = static_cast<FluentAccentColorButton *>(m_accentButtons[i]);
    if (!btn) {
      continue;
    }
    const QColor color =
        (i < m_accentColors.size()) ? m_accentColors[i] : QColor();
    btn->setColor(color);
    btn->setChecked(i == m_currentAccentIndex);
    btn->update();
  }
}

void FluentTitleBar::onAccentButtonClicked(int id) {
  if (id < 0 || id >= m_accentColors.size()) {
    return;
  }

  m_currentAccentIndex = id;
  const QColor color = m_accentColors[id];

  updateAccentButtons();

  if (color.isValid()) {
    qApp->setProperty("_q_accent_color", color);
  } else {
    qApp->setProperty("_q_accent_color", QVariant());
  }
#ifdef WIN32
  qApp->setStyle(QStringLiteral("FluentUI3"));
#else
  qApp->setStyle(new FluentUI3Style);
#endif

  emit accentColorChanged(color);
}
