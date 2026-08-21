#include "androidgallerywindow.h"

#include "androidintegration.h"

#include "excombobox.h"
#include "exexpander.h"
#include "exinfobar.h"
#include "exliquidgauge.h"
#include "exmultiprogressring.h"
#include "exprogressring.h"
#include "exradialgauge.h"
#include "exrangeslider.h"
#include "extimeline.h"
#include "fluentui3style.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QScreen>
#include <QScroller>
#include <QScrollerProperties>
#include <QShowEvent>
#include <QSlider>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStorageInfo>
#include <QStyle>
#include <QStringList>
#include <QSysInfo>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QUrl>
#include <QWindow>
#include <QtMath>

namespace {

constexpr int kTouchTarget = 48;
constexpr int kPageSpacing = 14;

QLabel *createTextLabel(const QString &text, bool secondary = false, QWidget *parent = nullptr)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    if (secondary)
        label->setProperty("isSecondaryText", true);
    return label;
}

QFrame *createCard(const QString &title, const QString &description = QString())
{
    auto *card = new QFrame;
    card->setProperty("isCard", true);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *titleLabel = createTextLabel(title, false, card);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    if (!description.isEmpty())
        layout->addWidget(createTextLabel(description, true, card));

    return card;
}

QVBoxLayout *cardLayout(QFrame *card)
{
    return qobject_cast<QVBoxLayout *>(card->layout());
}

void makeTouchFriendly(QWidget *widget)
{
    widget->setMinimumHeight(kTouchTarget);
    widget->setSizePolicy(QSizePolicy::Expanding, widget->sizePolicy().verticalPolicy());
}

QWidget *centeredWidget(QWidget *widget)
{
    auto *container = new QWidget;
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(widget);
    layout->addStretch();
    return container;
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 0)
        return QObject::tr("不可用");

    static const QStringList units{
        QObject::tr("B"), QObject::tr("KB"), QObject::tr("MB"),
        QObject::tr("GB"), QObject::tr("TB")
    };
    qreal value = bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1)
    {
        value /= 1024.0;
        ++unit;
    }
    const int precision = unit == 0 ? 0 : (value < 10.0 ? 2 : 1);
    return QObject::tr("%1 %2").arg(value, 0, 'f', precision).arg(units.at(unit));
}

QString formatRate(qint64 bytesPerSecond)
{
    return QObject::tr("%1/s").arg(formatBytes(bytesPerSecond));
}

int logarithmicRateProgress(qint64 bytesPerSecond)
{
    if (bytesPerSecond <= 0)
        return 0;

    // 1 KB/s 到约 100 MB/s 映射到 1–100
    const qreal normalized = qLn(1.0 + bytesPerSecond / 1024.0) / qLn(1.0 + 100.0 * 1024.0);
    return qBound(1, qRound(normalized * 100.0), 100);
}

// 移动端标准底部导航栏项（Fluent 图标在上，文字在下）
class BottomNavButton final : public QToolButton
{
public:
    BottomNavButton(const QString &glyph, const QString &text, QWidget *parent = nullptr)
        : QToolButton(parent)
        , m_glyph(glyph)
        , m_label(text)
    {
        setCheckable(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumHeight(56);
        setCursor(Qt::PointingHandCursor);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(2, 6, 2, 6);
        layout->setSpacing(2);
        layout->setAlignment(Qt::AlignCenter);

        m_iconLabel = new QLabel(m_glyph, this);
        QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
        iconFont.setPixelSize(18);
        m_iconLabel->setFont(iconFont);
        m_iconLabel->setAlignment(Qt::AlignCenter);
        m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(m_iconLabel);

        m_textLabel = new QLabel(m_label, this);
        QFont textFont = m_textLabel->font();
        textFont.setPixelSize(11);
        m_textLabel->setFont(textFont);
        m_textLabel->setAlignment(Qt::AlignCenter);
        m_textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(m_textLabel);

        updateVisualState();
    }

    void updateVisualState()
    {
        const bool checked = isChecked();
        const QPalette pal = palette();
        const QColor activeColor = pal.color(QPalette::Highlight);
        const QColor inactiveColor = pal.color(QPalette::PlaceholderText);

        if (m_iconLabel && m_textLabel)
        {
            QPalette iconPal = m_iconLabel->palette();
            iconPal.setColor(QPalette::WindowText, checked ? activeColor : inactiveColor);
            m_iconLabel->setPalette(iconPal);

            QPalette textPal = m_textLabel->palette();
            textPal.setColor(QPalette::WindowText, checked ? activeColor : inactiveColor);
            m_textLabel->setPalette(textPal);

            QFont f = m_textLabel->font();
            f.setBold(checked);
            m_textLabel->setFont(f);
        }
    }

protected:
    void checkStateSet() override
    {
        QToolButton::checkStateSet();
        updateVisualState();
    }

    void nextCheckState() override
    {
        QToolButton::nextCheckState();
        updateVisualState();
    }

    void changeEvent(QEvent *event) override
    {
        QToolButton::changeEvent(event);
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        {
            updateVisualState();
        }
    }

private:
    QString m_glyph;
    QString m_label;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_textLabel = nullptr;
};

} // namespace

AndroidGalleryWindow::AndroidGalleryWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("AndroidGalleryWindow"));
    setWindowTitle(tr("Fluent Android Gallery"));
    setCentralWidget(createShell());
    setCurrentPage(0);
}

QWidget *AndroidGalleryWindow::createShell()
{
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("android-gallery-root"));

    m_rootLayout = new QVBoxLayout(root);
    m_rootLayout->setContentsMargins(16, 10, 16, 8);
    m_rootLayout->setSpacing(10);

    auto *header = new QWidget(root);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(2, 2, 2, 2);
    headerLayout->setSpacing(10);

    auto *titles = new QWidget(header);
    auto *titlesLayout = new QVBoxLayout(titles);
    titlesLayout->setContentsMargins(0, 0, 0, 0);
    titlesLayout->setSpacing(1);

    m_pageTitle = new QLabel(titles);
    QFont pageTitleFont = m_pageTitle->font();
    pageTitleFont.setPixelSize(23);
    pageTitleFont.setBold(true);
    m_pageTitle->setFont(pageTitleFont);
    titlesLayout->addWidget(m_pageTitle);

    auto *subtitle = createTextLabel(tr("为触控和小屏幕重新设计"), true, titles);
    titlesLayout->addWidget(subtitle);

    headerLayout->addWidget(titles, 1);

    m_themeButton = new QToolButton(header);
    m_themeButton->setText(tr("深色"));
    m_themeButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_themeButton->setMinimumSize(72, kTouchTarget);
    connect(m_themeButton, &QToolButton::clicked, this, [this] {
        applyColorScheme(m_darkTheme ? 0 : 1);
    });
    headerLayout->addWidget(m_themeButton);

    m_rootLayout->addWidget(header);

    m_pages = new QStackedWidget(root);
    m_pages->addWidget(createHomePage());
    m_pages->addWidget(createControlsPage());
    m_pages->addWidget(createSystemMonitorPage());
    m_pages->addWidget(createAndroidPage());
    m_pages->addWidget(createSettingsPage());
    m_rootLayout->addWidget(m_pages, 1);

    auto *navigation = new QFrame(root);
    navigation->setProperty("isCard", true);
    auto *navigationLayout = new QHBoxLayout(navigation);
    navigationLayout->setContentsMargins(6, 4, 6, 4);
    navigationLayout->setSpacing(4);

    m_navigationGroup = new QButtonGroup(this);
    m_navigationGroup->setExclusive(true);

    struct NavItem
    {
        QString glyph;
        QString text;
    };
    const QList<NavItem> navItems{
        {QStringLiteral("\uE80F"), tr("首页")},
        {QStringLiteral("\uE790"), tr("控件")},
        {QStringLiteral("\uE9D9"), tr("监控")},
        {QStringLiteral("\uE770"), tr("系统")},
        {QStringLiteral("\uE713"), tr("设置")}
    };

    for (int index = 0; index < navItems.size(); ++index)
    {
        const auto &item = navItems.at(index);
        auto *button = new BottomNavButton(item.glyph, item.text, navigation);
        m_navigationGroup->addButton(button, index);
        navigationLayout->addWidget(button, 1);
    }
    connect(m_navigationGroup, &QButtonGroup::idClicked,
            this, &AndroidGalleryWindow::setCurrentPage);

    m_rootLayout->addWidget(navigation);
    return root;
}

QWidget *AndroidGalleryWindow::createHomePage()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 0, 12);
    layout->setSpacing(kPageSpacing);

    auto *hero = createCard(tr("Fluent UI，移动起来"),
                            tr("保留现有 Gallery 的视觉语言，改用适合拇指操作的尺寸、单列信息层级和底部导航。"));
    auto *exploreButton = new QPushButton(tr("浏览触控控件"), hero);
    exploreButton->setProperty("accent", true);
    makeTouchFriendly(exploreButton);
    connect(exploreButton, &QPushButton::clicked, this, [this] { setCurrentPage(1); });
    cardLayout(hero)->addWidget(exploreButton);
    auto *androidButton = new QPushButton(tr("体验 Android 系统功能"), hero);
    makeTouchFriendly(androidButton);
    connect(androidButton, &QPushButton::clicked, this, [this] { setCurrentPage(3); });
    cardLayout(hero)->addWidget(androidButton);
    layout->addWidget(hero);

    auto *status = new ExInfoBar(content);
    status->setSeverity(ExInfoBar::Success);
    status->setTitle(tr("Android 模式已启用"));
    status->setMessage(tr("桌面标题栏与固定窗口尺寸已关闭，支持触控惯性滑动并自动避开系统安全区。"));
    status->setClosable(false);
    status->setOpen(true);
    layout->addWidget(status);

    auto *principles = createCard(tr("移动端设计原则"));
    auto *principlesLayout = cardLayout(principles);
    principlesLayout->addWidget(createTextLabel(tr("• 48dp 级触控目标，减少误触"), true, principles));
    principlesLayout->addWidget(createTextLabel(tr("• 惯性手势滚动与动量回弹，顺畅浏览"), true, principles));
    principlesLayout->addWidget(createTextLabel(tr("• 单列响应式卡片，避免横向拥挤"), true, principles));
    principlesLayout->addWidget(createTextLabel(tr("• 底部导航栏，核心功能单手可达"), true, principles));
    layout->addWidget(principles);

    auto *expander = new ExExpander(content);
    expander->setHeader(tr("为什么不直接缩放桌面 Gallery？"));
    auto *explanation = new QWidget;
    auto *explanationLayout = new QVBoxLayout(explanation);
    explanationLayout->setContentsMargins(12, 8, 12, 12);
    explanationLayout->addWidget(createTextLabel(
        tr("桌面 Gallery 依赖宽屏导航、鼠标悬停和固定预览尺寸。移动版复用控件与主题，重新组织交互层级。"),
        true, explanation));
    expander->addContentWidget(explanation);
    expander->setExpanded(true);
    layout->addWidget(expander);

    layout->addStretch();
    return createScrollPage(content);
}

QWidget *AndroidGalleryWindow::createControlsPage()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 0, 12);
    layout->setSpacing(kPageSpacing);

    // 1. 按钮卡片
    auto *buttons = createCard(tr("按钮"), tr("主要操作突出显示，次要操作保持安静。"));
    auto *primaryButton = new QPushButton(tr("主要操作"), buttons);
    primaryButton->setProperty("accent", true);
    makeTouchFriendly(primaryButton);
    cardLayout(buttons)->addWidget(primaryButton);

    auto *buttonRow = new QWidget(buttons);
    auto *buttonRowLayout = new QHBoxLayout(buttonRow);
    buttonRowLayout->setContentsMargins(0, 0, 0, 0);
    auto *secondaryButton = new QPushButton(tr("次要操作"), buttonRow);
    auto *disabledButton = new QPushButton(tr("不可用"), buttonRow);
    disabledButton->setEnabled(false);
    makeTouchFriendly(secondaryButton);
    makeTouchFriendly(disabledButton);
    buttonRowLayout->addWidget(secondaryButton);
    buttonRowLayout->addWidget(disabledButton);
    cardLayout(buttons)->addWidget(buttonRow);
    layout->addWidget(buttons);

    // 2. 输入控件
    auto *inputs = createCard(tr("输入"), tr("输入控件使用整行宽度，便于软键盘和触控操作。"));
    auto *lineEdit = new QLineEdit(inputs);
    lineEdit->setPlaceholderText(tr("搜索组件"));
    lineEdit->setClearButtonEnabled(true);
    makeTouchFriendly(lineEdit);
    cardLayout(inputs)->addWidget(lineEdit);

    auto *comboBox = new ExComboBox(inputs);
    comboBox->addItems({tr("默认选项"), tr("触控优化"), tr("高对比度")});
    makeTouchFriendly(comboBox);
    cardLayout(inputs)->addWidget(comboBox);

    auto *spinBox = new QSpinBox(inputs);
    spinBox->setRange(0, 100);
    spinBox->setValue(42);
    spinBox->setSuffix(tr(" %"));
    makeTouchFriendly(spinBox);
    cardLayout(inputs)->addWidget(spinBox);
    layout->addWidget(inputs);

    // 3. 选择控件
    auto *selection = createCard(tr("选择"));
    auto *switchButton = new QCheckBox(tr("启用实时预览"), selection);
    switchButton->setProperty("isSwitchButton", true);
    switchButton->setChecked(true);
    makeTouchFriendly(switchButton);
    cardLayout(selection)->addWidget(switchButton);

    auto *checkBox = new QCheckBox(tr("记住我的选择"), selection);
    checkBox->setChecked(true);
    makeTouchFriendly(checkBox);
    cardLayout(selection)->addWidget(checkBox);

    auto *radioRow = new QWidget(selection);
    auto *radioLayout = new QHBoxLayout(radioRow);
    radioLayout->setContentsMargins(0, 0, 0, 0);
    auto *comfortable = new QRadioButton(tr("舒适"), radioRow);
    auto *compact = new QRadioButton(tr("紧凑"), radioRow);
    comfortable->setChecked(true);
    makeTouchFriendly(comfortable);
    makeTouchFriendly(compact);
    radioLayout->addWidget(comfortable);
    radioLayout->addWidget(compact);
    cardLayout(selection)->addWidget(radioRow);
    layout->addWidget(selection);

    // 4. 滑块控件
    auto *sliders = createCard(tr("滑块"));
    auto *valueLabel = createTextLabel(tr("音量：64"), true, sliders);
    cardLayout(sliders)->addWidget(valueLabel);
    auto *slider = new QSlider(Qt::Horizontal, sliders);
    slider->setRange(0, 100);
    slider->setValue(64);
    slider->setProperty("showValueTip", true);
    slider->setMinimumHeight(52);
    connect(slider, &QSlider::valueChanged, valueLabel, [valueLabel](int value) {
        valueLabel->setText(QObject::tr("音量：%1").arg(value));
    });
    cardLayout(sliders)->addWidget(slider);

    auto *rangeLabel = createTextLabel(tr("范围：20 – 80"), true, sliders);
    cardLayout(sliders)->addWidget(rangeLabel);
    auto *rangeSlider = new ExRangeSlider(Qt::Horizontal, sliders);
    rangeSlider->setRange(0, 100);
    rangeSlider->setValues(20, 80);
    rangeSlider->setProperty("showValueTip", true);
    rangeSlider->setMinimumHeight(58);
    connect(rangeSlider, &ExRangeSlider::valuesChanged, rangeLabel,
            [rangeLabel](int lower, int upper) {
                rangeLabel->setText(QObject::tr("范围：%1 – %2").arg(lower).arg(upper));
            });
    cardLayout(sliders)->addWidget(rangeSlider);
    layout->addWidget(sliders);

    // 5. ExLiquidGauge 水球波浪图
    auto *liquidCard = createCard(
        tr("水球波浪图 (ExLiquidGauge)"),
        tr("流动水波展示进度或容量，适合电量、存储和健康类移动端卡片。"));
    auto *liquidGauge = new ExLiquidGauge(liquidCard);
    liquidGauge->setRange(0, 100);
    liquidGauge->setValue(65);
    liquidGauge->setFixedSize(160, 160);
    cardLayout(liquidCard)->addWidget(centeredWidget(liquidGauge));

    auto *liquidSliderLabel = createTextLabel(tr("调节当前水位：65%"), true, liquidCard);
    cardLayout(liquidCard)->addWidget(liquidSliderLabel);
    auto *liquidSlider = new QSlider(Qt::Horizontal, liquidCard);
    liquidSlider->setRange(0, 100);
    liquidSlider->setValue(65);
    liquidSlider->setMinimumHeight(48);
    connect(liquidSlider, &QSlider::valueChanged, liquidGauge,
            [liquidGauge, liquidSliderLabel](int val) {
                liquidGauge->setValue(val);
                liquidSliderLabel->setText(QObject::tr("调节当前水位：%1%").arg(val));
            });
    cardLayout(liquidCard)->addWidget(liquidSlider);

    auto *shapeCombo = new ExComboBox(liquidCard);
    shapeCombo->addItem(tr("圆形轮廓 (Circle)"), QVariant::fromValue(int(ExLiquidGauge::CircleShape)));
    shapeCombo->addItem(tr("矩形轮廓 (Rect)"), QVariant::fromValue(int(ExLiquidGauge::RectShape)));
    shapeCombo->addItem(tr("水滴气泡 (Pin)"), QVariant::fromValue(int(ExLiquidGauge::PinShape)));
    shapeCombo->addItem(tr("三角轮廓 (Triangle)"), QVariant::fromValue(int(ExLiquidGauge::TriangleShape)));
    makeTouchFriendly(shapeCombo);
    connect(shapeCombo, qOverload<int>(&QComboBox::currentIndexChanged), liquidGauge,
            [liquidGauge, shapeCombo](int idx) {
                const auto shape = static_cast<ExLiquidGauge::Shape>(shapeCombo->itemData(idx).toInt());
                liquidGauge->setShape(shape);
            });
    cardLayout(liquidCard)->addWidget(shapeCombo);

    auto *waveAnimCheck = new QCheckBox(tr("波浪流动动画"), liquidCard);
    waveAnimCheck->setProperty("isSwitchButton", true);
    waveAnimCheck->setChecked(true);
    makeTouchFriendly(waveAnimCheck);
    connect(waveAnimCheck, &QCheckBox::toggled, liquidGauge, &ExLiquidGauge::setAnimationEnabled);
    cardLayout(liquidCard)->addWidget(waveAnimCheck);
    layout->addWidget(liquidCard);

    // 6. ExMultiProgressRing 多环进度条
    auto *multiRingCard = createCard(
        tr("多环进度环 (ExMultiProgressRing)"),
        tr("同心圆环展示多维指标，支持中心数值、徽标与平滑过渡。"));
    auto *multiRing = new ExMultiProgressRing(multiRingCard);
    multiRing->setRange(0, 100);
    multiRing->setFixedSize(180, 180);
    auto *activityItem = multiRing->addItem(tr("活动"), 78, QColor(QStringLiteral("#FF5722")));
    auto *exerciseItem = multiRing->addItem(tr("锻炼"), 55, QColor(QStringLiteral("#4CAF50")));
    auto *standItem = multiRing->addItem(tr("站立"), 90, QColor(QStringLiteral("#00BCD4")));
    cardLayout(multiRingCard)->addWidget(centeredWidget(multiRing));

    auto *randomRingButton = new QPushButton(tr("随机更新多环数值"), multiRingCard);
    randomRingButton->setProperty("accent", true);
    makeTouchFriendly(randomRingButton);
    connect(randomRingButton, &QPushButton::clicked, multiRing,
            [activityItem, exerciseItem, standItem] {
                auto *rng = QRandomGenerator::global();
                activityItem->setValue(rng->bounded(20, 100));
                exerciseItem->setValue(rng->bounded(10, 100));
                standItem->setValue(rng->bounded(30, 100));
            });
    cardLayout(multiRingCard)->addWidget(randomRingButton);
    layout->addWidget(multiRingCard);

    // 7. ExTimeline 移动端时间轴
    auto *timelineCard = createCard(
        tr("时间轴 (ExTimeline)"),
        tr("单列流式时间轴，适用于移动端任务节点、物流轨迹与版本日志。"));
    auto *timeline = new ExTimeline(timelineCard);
    timeline->setLayoutMode(ExTimeline::ContentOnRight);
    timeline->setMinimumHeight(240);
    timeline->addEvent(
        QDateTime::currentDateTime().addSecs(-7200),
        tr("应用构建完成"),
        tr("针对 Android ARM64 架构打包与签名"),
        ExTimelineEvent::Completed);
    timeline->addEvent(
        QDateTime::currentDateTime().addSecs(-3600),
        tr("触控手势配置"),
        tr("注入 QScroller 惯性滚动与回弹阻尼"),
        ExTimelineEvent::Completed);
    timeline->addEvent(
        QDateTime::currentDateTime(),
        tr("当前运行中"),
        tr("FluentUI3Style 移动主题渲染就绪"),
        ExTimelineEvent::Current);
    timeline->addEvent(
        QDateTime::currentDateTime().addSecs(3600),
        tr("后台资源优化"),
        tr("准备休眠和低功耗监听"),
        ExTimelineEvent::Pending);
    cardLayout(timelineCard)->addWidget(timeline);
    layout->addWidget(timelineCard);

    // 8. 即时反馈
    auto *feedback = createCard(tr("即时反馈"));
    auto *infoBar = new ExInfoBar(feedback);
    infoBar->setSeverity(ExInfoBar::Informational);
    infoBar->setTitle(tr("设置已保存"));
    infoBar->setMessage(tr("这是不会遮挡内容的移动端通知。"));
    infoBar->setOpen(false);
    cardLayout(feedback)->addWidget(infoBar);
    auto *showInfo = new QPushButton(tr("显示 InfoBar"), feedback);
    makeTouchFriendly(showInfo);
    connect(showInfo, &QPushButton::clicked, infoBar, [infoBar] {
        infoBar->setOpen(false);
        infoBar->setOpen(true);
    });
    cardLayout(feedback)->addWidget(showInfo);
    layout->addWidget(feedback);

    layout->addStretch();
    return createScrollPage(content);
}

QWidget *AndroidGalleryWindow::createSystemMonitorPage()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 0, 12);
    layout->setSpacing(kPageSpacing);

    auto *summary = new ExInfoBar(content);
    summary->setSeverity(ExInfoBar::Informational);
    summary->setTitle(tr("实时系统监控"));
    summary->setMessage(tr("数据来自 Android 系统接口；离开本页后自动停止轮询。"));
    summary->setClosable(false);
    summary->setOpen(true);
    layout->addWidget(summary);

    // 1. 综合指标多环卡片
    auto *multiCard = createCard(
        tr("整机综合健康环"),
        tr("外环：内存占用 · 中环：电池电量 · 内环：存储空间"));
    m_systemMultiRing = new ExMultiProgressRing(multiCard);
    m_systemMultiRing->setRange(0, 100);
    m_systemMultiRing->setFixedSize(200, 200);
    m_systemMultiRing->addItem(tr("内存"), 0, QColor(QStringLiteral("#FF5252")));
    m_systemMultiRing->addItem(tr("电池"), 0, QColor(QStringLiteral("#4CAF50")));
    m_systemMultiRing->addItem(tr("存储"), 0, QColor(QStringLiteral("#2196F3")));
    cardLayout(multiCard)->addWidget(centeredWidget(m_systemMultiRing));
    layout->addWidget(multiCard);

    // 2. 运行内存表盘
    auto *memoryCard = createCard(
        tr("运行内存"), tr("仪表盘显示整机已用内存比例，并标记 Android 的低内存状态。"));
    m_memoryGauge = new ExRadialGauge(memoryCard);
    m_memoryGauge->setRange(0, 100);
    m_memoryGauge->setValue(0);
    m_memoryGauge->setTitle(tr("内存"));
    m_memoryGauge->setUnit(QStringLiteral("%"));
    m_memoryGauge->setInteractive(false);
    m_memoryGauge->setNeedleStyle(ExRadialGauge::NoNeedle);
    m_memoryGauge->setLabelsVisible(true);
    m_memoryGauge->setFixedSize(220, 220);
    cardLayout(memoryCard)->addWidget(centeredWidget(m_memoryGauge));
    m_memoryDetails = createTextLabel(tr("正在读取内存信息…"), true, memoryCard);
    m_memoryDetails->setAlignment(Qt::AlignCenter);
    cardLayout(memoryCard)->addWidget(m_memoryDetails);
    layout->addWidget(memoryCard);

    // 3. 实时网速
    auto *networkCard = createCard(
        tr("实时网速"),
        tr("根据 Android TrafficStats 的累计流量计算整机当前下载和上传速度。"));
    m_downloadDetails = createTextLabel(tr("下载：等待下一次采样…"), true, networkCard);
    cardLayout(networkCard)->addWidget(m_downloadDetails);
    m_downloadProgress = new QProgressBar(networkCard);
    m_downloadProgress->setRange(0, 100);
    m_downloadProgress->setValue(0);
    m_downloadProgress->setTextVisible(false);
    m_downloadProgress->setMinimumHeight(12);
    cardLayout(networkCard)->addWidget(m_downloadProgress);

    m_uploadDetails = createTextLabel(tr("上传：等待下一次采样…"), true, networkCard);
    cardLayout(networkCard)->addWidget(m_uploadDetails);
    m_uploadProgress = new QProgressBar(networkCard);
    m_uploadProgress->setRange(0, 100);
    m_uploadProgress->setValue(0);
    m_uploadProgress->setTextVisible(false);
    m_uploadProgress->setMinimumHeight(12);
    cardLayout(networkCard)->addWidget(m_uploadProgress);
    layout->addWidget(networkCard);

    // 4. 存储与水波容量卡片
    auto *storageCard = createCard(
        tr("存储空间 (水波容量展示)"),
        tr("水波图显示应用所在存储卷的使用情况。"));
    m_storageLiquidGauge = new ExLiquidGauge(storageCard);
    m_storageLiquidGauge->setRange(0, 100);
    m_storageLiquidGauge->setValue(0);
    m_storageLiquidGauge->setFixedSize(150, 150);
    cardLayout(storageCard)->addWidget(centeredWidget(m_storageLiquidGauge));

    m_storageDetails = createTextLabel(tr("正在读取存储信息…"), true, storageCard);
    m_storageDetails->setAlignment(Qt::AlignCenter);
    cardLayout(storageCard)->addWidget(m_storageDetails);
    m_storageProgress = new QProgressBar(storageCard);
    m_storageProgress->setRange(0, 100);
    m_storageProgress->setValue(0);
    m_storageProgress->setFormat(tr("已使用 %p%"));
    m_storageProgress->setMinimumHeight(28);
    cardLayout(storageCard)->addWidget(m_storageProgress);
    layout->addWidget(storageCard);

    // 5. 电池状态
    auto *batteryCard = createCard(
        tr("电池状态"), tr("ProgressRing 呈现当前电量与充电连接。"));
    m_batteryRing = new ExProgressRing(batteryCard);
    m_batteryRing->setRange(0, 100);
    m_batteryRing->setValue(0);
    m_batteryRing->setTitle(tr("电池"));
    m_batteryRing->setFormat(QStringLiteral("%p%"));
    m_batteryRing->setFixedSize(160, 160);
    cardLayout(batteryCard)->addWidget(centeredWidget(m_batteryRing));
    m_batteryDetails = createTextLabel(tr("正在读取电池状态…"), true, batteryCard);
    m_batteryDetails->setAlignment(Qt::AlignCenter);
    cardLayout(batteryCard)->addWidget(m_batteryDetails);
    layout->addWidget(batteryCard);

    // 6. 采样设置
    auto *refreshCard = createCard(
        tr("采样设置"), tr("自动刷新仅在当前页面生效，以减少后台耗电。"));
    auto *autoRefresh = new QCheckBox(tr("自动刷新"), refreshCard);
    autoRefresh->setProperty("isSwitchButton", true);
    autoRefresh->setChecked(true);
    makeTouchFriendly(autoRefresh);
    connect(autoRefresh, &QCheckBox::toggled, this, [this](bool checked) {
        m_monitorAutoRefresh = checked;
        if (!m_monitorTimer)
            return;
        if (checked && m_pages && m_pages->currentIndex() == 2)
            m_monitorTimer->start();
        else
            m_monitorTimer->stop();
    });
    cardLayout(refreshCard)->addWidget(autoRefresh);

    auto *intervalLabel = createTextLabel(tr("刷新间隔"), true, refreshCard);
    cardLayout(refreshCard)->addWidget(intervalLabel);
    auto *intervalCombo = new ExComboBox(refreshCard);
    intervalCombo->addItem(tr("1 秒"), 1000);
    intervalCombo->addItem(tr("2 秒"), 2000);
    intervalCombo->addItem(tr("5 秒"), 5000);
    intervalCombo->setCurrentIndex(1);
    intervalCombo->setAccessibleName(tr("刷新间隔"));
    makeTouchFriendly(intervalCombo);
    cardLayout(refreshCard)->addWidget(intervalCombo);

    auto *refreshButton = new QPushButton(tr("立即刷新"), refreshCard);
    refreshButton->setProperty("accent", true);
    makeTouchFriendly(refreshButton);
    cardLayout(refreshCard)->addWidget(refreshButton);
    layout->addWidget(refreshCard);

    m_monitorStatus = new ExInfoBar(content);
    m_monitorStatus->setClosable(false);
    m_monitorStatus->setOpen(false);
    layout->addWidget(m_monitorStatus);

    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(2000);
    connect(m_monitorTimer, &QTimer::timeout, this, &AndroidGalleryWindow::updateSystemMonitor);
    connect(intervalCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, intervalCombo](int index) {
                const int interval = intervalCombo->itemData(index).toInt();
                if (interval > 0 && m_monitorTimer)
                    m_monitorTimer->setInterval(interval);
            });
    connect(refreshButton, &QPushButton::clicked,
            this, &AndroidGalleryWindow::updateSystemMonitor);

    layout->addStretch();
    return createScrollPage(content);
}

QWidget *AndroidGalleryWindow::createSettingsPage()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 0, 12);
    layout->setSpacing(kPageSpacing);

    auto *themeCard = createCard(tr("主题"), tr("切换后会让 FluentUI3Style 重新读取应用配色。"));
    auto *themeRow = new QWidget(themeCard);
    auto *themeLayout = new QHBoxLayout(themeRow);
    themeLayout->setContentsMargins(0, 0, 0, 0);
    auto *lightButton = new QPushButton(tr("浅色"), themeRow);
    auto *darkButton = new QPushButton(tr("深色"), themeRow);
    makeTouchFriendly(lightButton);
    makeTouchFriendly(darkButton);
    themeLayout->addWidget(lightButton);
    themeLayout->addWidget(darkButton);
    connect(lightButton, &QPushButton::clicked, this, [this] { applyColorScheme(0); });
    connect(darkButton, &QPushButton::clicked, this, [this] { applyColorScheme(1); });
    cardLayout(themeCard)->addWidget(themeRow);
    layout->addWidget(themeCard);

    auto *accentCard = createCard(tr("强调色"));
    const QList<QPair<QString, QColor>> accents{
        {tr("Fluent 经典蓝"), QColor(QStringLiteral("#0067C0"))},
        {tr("微软紫"), QColor(QStringLiteral("#744DA9"))},
        {tr("森林绿"), QColor(QStringLiteral("#0F7B0F"))},
        {tr("活力橙"), QColor(QStringLiteral("#D83B01"))},
        {tr("青瓷绿"), QColor(QStringLiteral("#038387"))}
    };
    for (const auto &accent : accents)
    {
        auto *button = new QPushButton(accent.first, accentCard);
        button->setProperty("accent", true);
        makeTouchFriendly(button);
        connect(button, &QPushButton::clicked, this,
                [this, color = accent.second] { applyAccentColor(color); });
        cardLayout(accentCard)->addWidget(button);
    }
    auto *resetAccent = new QPushButton(tr("跟随系统默认"), accentCard);
    makeTouchFriendly(resetAccent);
    connect(resetAccent, &QPushButton::clicked, this, [this] {
        qApp->setProperty("_q_accent_color", QVariant());
        qApp->setStyle(new FluentUI3Style);
    });
    cardLayout(accentCard)->addWidget(resetAccent);
    layout->addWidget(accentCard);

    auto *aboutCard = createCard(tr("关于这个 Demo"));
    cardLayout(aboutCard)->addWidget(createTextLabel(
        tr("AndroidGallery 是独立移动端入口，与桌面 Gallery 共用 FluentUI3Style 和 ExWidgets。"),
        true, aboutCard));
    cardLayout(aboutCard)->addWidget(createTextLabel(
        tr("包名：io.github.window11style.androidgallery\n版本：1.0.0"),
        true, aboutCard));
    layout->addWidget(aboutCard);

    layout->addStretch();
    return createScrollPage(content);
}

QWidget *AndroidGalleryWindow::createAndroidPage()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 0, 12);
    layout->setSpacing(kPageSpacing);

    auto *feedbackCard = createCard(
        tr("系统反馈"),
        tr("这些操作通过 Android 原生接口执行，不需要申请敏感权限。"));

    auto *toastButton = new QPushButton(tr("显示 Android Toast"), feedbackCard);
    toastButton->setProperty("accent", true);
    makeTouchFriendly(toastButton);
    connect(toastButton, &QPushButton::clicked, this, [this] {
        AndroidIntegration::showToast(QObject::tr("来自 Fluent Android Gallery 的问候"));
        addAndroidHistoryEvent(tr("弹出原生 Toast"), tr("展示轻量提示消息"));
    });
    cardLayout(feedbackCard)->addWidget(toastButton);

    auto *hapticButton = new QPushButton(tr("触发轻触反馈"), feedbackCard);
    makeTouchFriendly(hapticButton);
    connect(hapticButton, &QPushButton::clicked, this, [this] {
        AndroidIntegration::performHapticFeedback();
        addAndroidHistoryEvent(tr("触觉振动反馈"), tr("执行 HapticFeedbackConstants.KEYBOARD_TAP"));
    });
    cardLayout(feedbackCard)->addWidget(hapticButton);
    layout->addWidget(feedbackCard);

    auto *sharingCard = createCard(
        tr("分享与剪贴板"),
        tr("调用系统分享面板，也可以把演示文本复制到剪贴板。"));

    auto *shareButton = new QPushButton(tr("打开系统分享面板"), sharingCard);
    shareButton->setProperty("accent", true);
    makeTouchFriendly(shareButton);
    connect(shareButton, &QPushButton::clicked, this, [this] {
        AndroidIntegration::shareText(
            QObject::tr("Fluent Android Gallery"),
            QObject::tr("我正在体验 Window11Style 的 Android 移动端 Demo。"));
        addAndroidHistoryEvent(tr("调用系统分享"), tr("启动 Android Intent.ACTION_SEND"));
    });
    cardLayout(sharingCard)->addWidget(shareButton);

    auto *clipboardButton = new QPushButton(tr("复制演示文本"), sharingCard);
    makeTouchFriendly(clipboardButton);
    connect(clipboardButton, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(
            QObject::tr("Fluent Android Gallery · Window11Style"));
        AndroidIntegration::showToast(QObject::tr("已复制到剪贴板"));
        addAndroidHistoryEvent(tr("写入剪贴板"), tr("Fluent Android Gallery · Window11Style"));
    });
    cardLayout(sharingCard)->addWidget(clipboardButton);
    layout->addWidget(sharingCard);

    auto *externalCard = createCard(
        tr("系统应用"),
        tr("使用 Android 浏览器和系统文件选择器处理外部内容。"));

    auto *browserButton = new QPushButton(tr("在浏览器中打开 Qt Android 文档"), externalCard);
    makeTouchFriendly(browserButton);
    connect(browserButton, &QPushButton::clicked, this, [this] {
        const bool opened = QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://doc.qt.io/qt-6/android.html")));
        AndroidIntegration::showToast(
            opened ? QObject::tr("正在打开浏览器") : QObject::tr("没有可用的浏览器"));
        addAndroidHistoryEvent(tr("打开外部浏览器"), tr("https://doc.qt.io/qt-6/android.html"));
    });
    cardLayout(externalCard)->addWidget(browserButton);

    auto *selectedFile = createTextLabel(tr("尚未选择文件"), true, externalCard);
    auto *fileButton = new QPushButton(tr("使用系统文件选择器"), externalCard);
    makeTouchFriendly(fileButton);
    connect(fileButton, &QPushButton::clicked, this, [this, selectedFile] {
        const QUrl fileUrl = QFileDialog::getOpenFileUrl(
            this, tr("选择一个文件"), QUrl(), tr("所有文件 (*)"));
        if (fileUrl.isEmpty())
            return;
        selectedFile->setText(
            tr("已选择：%1\n%2").arg(fileUrl.fileName(), fileUrl.toDisplayString()));
        AndroidIntegration::showToast(tr("文件已选择"));
        addAndroidHistoryEvent(tr("选择系统文件"), fileUrl.fileName());
    });
    cardLayout(externalCard)->addWidget(fileButton);
    cardLayout(externalCard)->addWidget(selectedFile);
    layout->addWidget(externalCard);

    // 操作历程时间轴
    auto *historyCard = createCard(
        tr("交互操作历程 (ExTimeline)"),
        tr("记录当前会话调用的 Android 系统功能。"));
    m_historyTimeline = new ExTimeline(historyCard);
    m_historyTimeline->setLayoutMode(ExTimeline::ContentOnRight);
    m_historyTimeline->setMinimumHeight(180);
    m_historyTimeline->addEvent(
        QDateTime::currentDateTime(),
        tr("页面初始化"),
        tr("系统功能展示就绪"),
        ExTimelineEvent::Completed);
    cardLayout(historyCard)->addWidget(m_historyTimeline);
    layout->addWidget(historyCard);

    auto *deviceCard = createCard(
        tr("设备信息"),
        tr("展示 Qt 和 Android 当前可见的屏幕与运行环境信息。"));
    auto *deviceLabel = createTextLabel(deviceSummary(), true, deviceCard);
    deviceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cardLayout(deviceCard)->addWidget(deviceLabel);

    auto *refreshButton = new QPushButton(tr("刷新设备信息"), deviceCard);
    makeTouchFriendly(refreshButton);
    connect(refreshButton, &QPushButton::clicked, this, [this, deviceLabel] {
        deviceLabel->setText(deviceSummary());
        AndroidIntegration::showToast(tr("设备信息已刷新"));
        addAndroidHistoryEvent(tr("刷新设备环境"), tr("读取屏幕与系统安全区数据"));
    });
    cardLayout(deviceCard)->addWidget(refreshButton);
    layout->addWidget(deviceCard);

    auto *backHint = new ExInfoBar(content);
    backHint->setSeverity(ExInfoBar::Informational);
    backHint->setTitle(tr("返回键已适配"));
    backHint->setMessage(tr("在任意子页面按 Android 返回键会先回到首页，再按一次退出应用。"));
    backHint->setClosable(false);
    backHint->setOpen(true);
    layout->addWidget(backHint);

    layout->addStretch();
    return createScrollPage(content);
}

void AndroidGalleryWindow::addAndroidHistoryEvent(const QString &title, const QString &description)
{
    if (!m_historyTimeline)
        return;

    // 将之前的事件都标记为 Completed，新增的标记为 Current
    const auto events = m_historyTimeline->events();
    for (auto *ev : events)
    {
        if (ev->status() == ExTimelineEvent::Current)
            ev->setStatus(ExTimelineEvent::Completed);
    }

    m_historyTimeline->addEvent(
        QDateTime::currentDateTime(),
        title,
        description,
        ExTimelineEvent::Current);
}

QWidget *AndroidGalleryWindow::createScrollPage(QWidget *content) const
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(content);
    scrollArea->viewport()->setAutoFillBackground(false);

    // 启用 QScroller 触控动量与鼠标拖拽惯性滑动（类似原生 Android 滚动）
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);

    QScroller *scroller = QScroller::scroller(scrollArea->viewport());
    if (scroller)
    {
        QScrollerProperties props = scroller->scrollerProperties();
        props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                              QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
        props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                              QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
        props.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.0);
        props.setScrollMetric(QScrollerProperties::FrameRate,
                              QVariant::fromValue(QScrollerProperties::Fps60));
        scroller->setScrollerProperties(props);
    }

    return scrollArea;
}

void AndroidGalleryWindow::setCurrentPage(int index)
{
    if (!m_pages || index < 0 || index >= m_pages->count())
        return;

    static const QStringList pageTitles{
        tr("Fluent Gallery"), tr("触控控件"), tr("系统监控"),
        tr("Android 系统"), tr("个性化")
    };

    m_pages->setCurrentIndex(index);
    m_pageTitle->setText(pageTitles.at(index));
    if (auto *button = m_navigationGroup->button(index))
        button->setChecked(true);

    if (m_monitorTimer)
    {
        if (index == 2)
        {
            m_lastMonitorSampleMs = 0;
            m_lastReceivedBytes = -1;
            m_lastTransmittedBytes = -1;
            updateSystemMonitor();
            if (m_monitorAutoRefresh)
                m_monitorTimer->start();
        }
        else
        {
            m_monitorTimer->stop();
            m_lastMonitorSampleMs = 0;
        }
    }
}

QString AndroidGalleryWindow::deviceSummary() const
{
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    const QSize screenSize = screen ? screen->size() : QSize();
    const qreal density = screen ? screen->devicePixelRatio() : 1.0;
    const qreal dpi = screen ? screen->physicalDotsPerInch() : 0.0;
    const QMargins safeArea = windowHandle() ? windowHandle()->safeAreaMargins() : QMargins();

    return tr("Android API：%1\n"
              "设备：%2\n"
              "CPU：%3\n"
              "内核：%4 %5\n"
              "屏幕：%6 × %7，DPR %8，%9 DPI\n"
              "安全区：左 %10 / 上 %11 / 右 %12 / 下 %13")
        .arg(AndroidIntegration::sdkVersion())
        .arg(QSysInfo::prettyProductName())
        .arg(QSysInfo::currentCpuArchitecture())
        .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion())
        .arg(screenSize.width())
        .arg(screenSize.height())
        .arg(density, 0, 'f', 2)
        .arg(dpi, 0, 'f', 0)
        .arg(safeArea.left())
        .arg(safeArea.top())
        .arg(safeArea.right())
        .arg(safeArea.bottom());
}

void AndroidGalleryWindow::updateSystemMonitor()
{
    const AndroidIntegration::SystemSnapshot snapshot =
        AndroidIntegration::readSystemSnapshot();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    int memPercent = 0;
    if (m_memoryGauge && m_memoryDetails)
    {
        if (snapshot.totalMemoryBytes > 0 && snapshot.availableMemoryBytes >= 0)
        {
            const qint64 used = qMax<qint64>(
                0, snapshot.totalMemoryBytes - snapshot.availableMemoryBytes);
            memPercent = qBound(
                0, qRound(100.0 * used / snapshot.totalMemoryBytes), 100);
            m_memoryGauge->setValue(memPercent);
            m_memoryDetails->setText(
                tr("已用 %1 / 总计 %2\n可用 %3 · 低内存阈值 %4")
                    .arg(formatBytes(used), formatBytes(snapshot.totalMemoryBytes),
                         formatBytes(snapshot.availableMemoryBytes),
                         formatBytes(snapshot.lowMemoryThresholdBytes)));
        }
        else
        {
            m_memoryGauge->setValue(0);
            m_memoryDetails->setText(tr("当前设备未返回内存信息"));
        }
    }

    qint64 downloadRate = -1;
    qint64 uploadRate = -1;
    if (m_lastMonitorSampleMs > 0 && now > m_lastMonitorSampleMs
        && snapshot.totalReceivedBytes >= m_lastReceivedBytes
        && snapshot.totalTransmittedBytes >= m_lastTransmittedBytes)
    {
        const qint64 elapsed = now - m_lastMonitorSampleMs;
        downloadRate = (snapshot.totalReceivedBytes - m_lastReceivedBytes) * 1000 / elapsed;
        uploadRate = (snapshot.totalTransmittedBytes - m_lastTransmittedBytes) * 1000 / elapsed;
    }

    if (snapshot.totalReceivedBytes >= 0 && snapshot.totalTransmittedBytes >= 0)
    {
        m_lastReceivedBytes = snapshot.totalReceivedBytes;
        m_lastTransmittedBytes = snapshot.totalTransmittedBytes;
        m_lastMonitorSampleMs = now;
    }

    if (m_downloadDetails && m_uploadDetails && m_downloadProgress && m_uploadProgress)
    {
        if (downloadRate >= 0 && uploadRate >= 0)
        {
            m_downloadDetails->setText(
                tr("下载：%1 · 开机后累计 %2")
                    .arg(formatRate(downloadRate), formatBytes(snapshot.totalReceivedBytes)));
            m_uploadDetails->setText(
                tr("上传：%1 · 开机后累计 %2")
                    .arg(formatRate(uploadRate), formatBytes(snapshot.totalTransmittedBytes)));
            m_downloadProgress->setValue(logarithmicRateProgress(downloadRate));
            m_uploadProgress->setValue(logarithmicRateProgress(uploadRate));
        }
        else if (snapshot.totalReceivedBytes >= 0 && snapshot.totalTransmittedBytes >= 0)
        {
            m_downloadDetails->setText(tr("下载：等待下一次采样…"));
            m_uploadDetails->setText(tr("上传：等待下一次采样…"));
            m_downloadProgress->setValue(0);
            m_uploadProgress->setValue(0);
        }
        else
        {
            m_downloadDetails->setText(tr("下载：设备不支持 TrafficStats 总流量"));
            m_uploadDetails->setText(tr("上传：设备不支持 TrafficStats 总流量"));
            m_downloadProgress->setValue(0);
            m_uploadProgress->setValue(0);
        }
    }

    int batteryVal = 0;
    if (m_batteryRing && m_batteryDetails)
    {
        if (snapshot.batteryLevel >= 0)
        {
            batteryVal = snapshot.batteryLevel;
            m_batteryRing->setValue(batteryVal);
            m_batteryRing->setTitle(snapshot.charging ? tr("充电中") : tr("电池"));
            m_batteryDetails->setText(
                snapshot.charging ? tr("已连接电源") : tr("正在使用电池"));
        }
        else
        {
            m_batteryRing->setValue(0);
            m_batteryRing->setTitle(tr("电池"));
            m_batteryDetails->setText(tr("当前设备未返回电池状态"));
        }
    }

    int storagePercent = 0;
    if (m_storageProgress && m_storageDetails)
    {
        const QStorageInfo storage = QStorageInfo::root();
        if (storage.isValid() && storage.isReady() && storage.bytesTotal() > 0)
        {
            const qint64 used = qMax<qint64>(0, storage.bytesTotal() - storage.bytesAvailable());
            storagePercent = qBound(
                0, qRound(100.0 * used / storage.bytesTotal()), 100);
            m_storageProgress->setValue(storagePercent);
            if (m_storageLiquidGauge)
                m_storageLiquidGauge->setValue(storagePercent);
            m_storageDetails->setText(
                tr("存储：已用 %1 / 总计 %2 · 可用 %3")
                    .arg(formatBytes(used), formatBytes(storage.bytesTotal()),
                         formatBytes(storage.bytesAvailable())));
        }
        else
        {
            m_storageProgress->setValue(0);
            if (m_storageLiquidGauge)
                m_storageLiquidGauge->setValue(0);
            m_storageDetails->setText(tr("当前设备未返回存储信息"));
        }
    }

    // 更新综合多环
    if (m_systemMultiRing)
    {
        const auto items = m_systemMultiRing->items();
        if (items.size() >= 3)
        {
            items.at(0)->setValue(memPercent);
            items.at(1)->setValue(batteryVal);
            items.at(2)->setValue(storagePercent);
        }
    }

    if (m_monitorStatus)
    {
        if (snapshot.lowMemory)
        {
            m_monitorStatus->setSeverity(ExInfoBar::Warning);
            m_monitorStatus->setTitle(tr("Android 报告低内存"));
            m_monitorStatus->setMessage(tr("建议释放缓存或关闭暂时不用的应用。"));
        }
        else if (snapshot.totalMemoryBytes <= 0 || snapshot.totalReceivedBytes < 0)
        {
            m_monitorStatus->setSeverity(ExInfoBar::Warning);
            m_monitorStatus->setTitle(tr("部分系统信息不可用"));
            m_monitorStatus->setMessage(tr("不同 Android 厂商可能限制部分统计接口。"));
        }
        else
        {
            m_monitorStatus->setSeverity(ExInfoBar::Success);
            m_monitorStatus->setTitle(tr("系统状态正常"));
            m_monitorStatus->setMessage(
                tr("采样时间：%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))));
        }
        m_monitorStatus->setOpen(true);
    }
}

void AndroidGalleryWindow::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Back || event->key() == Qt::Key_Escape)
        && m_pages && m_pages->currentIndex() != 0)
    {
        setCurrentPage(0);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void AndroidGalleryWindow::closeEvent(QCloseEvent *event)
{
    if (m_pages && m_pages->currentIndex() != 0)
    {
        setCurrentPage(0);
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);
}

void AndroidGalleryWindow::applyColorScheme(int colorScheme)
{
    m_darkTheme = colorScheme == 1;
    qApp->setProperty("_q_colorscheme", colorScheme);
    qApp->setStyle(new FluentUI3Style);
    m_themeButton->setText(m_darkTheme ? tr("浅色") : tr("深色"));
}

void AndroidGalleryWindow::applyAccentColor(const QColor &color)
{
    qApp->setProperty("_q_accent_color", color);
    qApp->setStyle(new FluentUI3Style);
}

void AndroidGalleryWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (QWindow *handle = windowHandle())
    {
        if (!m_safeAreaConnected)
        {
            connect(handle, &QWindow::safeAreaMarginsChanged, this,
                    [this](QMargins) { updateSafeAreaMargins(); });
            m_safeAreaConnected = true;
        }
        updateSafeAreaMargins();
    }
}

void AndroidGalleryWindow::updateSafeAreaMargins()
{
    if (!m_rootLayout || !windowHandle())
        return;

    const QMargins safeArea = windowHandle()->safeAreaMargins();
    m_rootLayout->setContentsMargins(
        16 + safeArea.left(),
        10 + safeArea.top(),
        16 + safeArea.right(),
        8 + safeArea.bottom());
}
