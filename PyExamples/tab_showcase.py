from PySide6.QtWidgets import QVBoxLayout, QScrollArea, QWidget, QLabel, QTabWidget, QTabBar, QFrame
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont, QColor
from utils import create_fluent_icon

def setup_tab_showcase(window, main_controller):
    page4_layout = QVBoxLayout(window.page_4)
    page4_layout.setContentsMargins(0, 0, 0, 0)
    page4_layout.setSpacing(0)
    
    scroll = QScrollArea()
    scroll.setWidgetResizable(True)
    scroll.setFrameShape(QFrame.StyledPanel)
    scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
    scroll.viewport().setAutoFillBackground(False)
    scroll.viewport().setAttribute(Qt.WA_StyledBackground, False)
    
    # 容器
    content_widget = QWidget()
    content_widget.setAutoFillBackground(False)
    scroll.setWidget(content_widget)
    page4_layout.addWidget(scroll)
    
    main_layout = QVBoxLayout(content_widget)
    main_layout.setContentsMargins(11, 11, 11, 11)
    main_layout.setSpacing(15)
    
    main_title = QLabel("TabBar多种样式示例")
    mt_font = main_title.font()
    mt_font.setBold(True)
    mt_font.setPointSize(18)
    main_title.setFont(mt_font)
    main_layout.addWidget(main_title)

    def create_card_container():
        card = QWidget()
        card.setProperty("isCard", True)
        card.setAttribute(Qt.WA_StyledBackground)
        layout = QVBoxLayout(card)
        layout.setSpacing(10)
        layout.setContentsMargins(10, 10, 10, 10)
        return card, layout
        
    def add_section_title(layout, title, desc=""):
        t_label = QLabel(title)
        tf = t_label.font()
        tf.setBold(True)
        tf.setPixelSize(14)
        t_label.setFont(tf)
        layout.addWidget(t_label)
        
        if desc:
            d_label = QLabel(desc)
            d_label.setStyleSheet("color: gray; font-size: 12px;")
            layout.addWidget(d_label)

    # ================= 1. Pivot Tabs =================
    pivot_card, pivot_layout = create_card_container()
    
    def add_standard_bar(layout, title, desc, style_enum):
        add_section_title(layout, title, desc)
        bar = QTabBar()
        bar.setDrawBase(False)
        bar.setExpanding(False)
        bar.setProperty("tabBarStyle", style_enum)
        bar.addTab("Home")
        bar.addTab("Search")
        bar.addTab("Settings")
        bar.addTab("Help")
        bar.addTab("About")
        layout.addWidget(bar)
        return bar

    add_standard_bar(pivot_layout, "Pivot Grow TabBar", "特点：选中时会有一个生长动画效果。", 2)
    add_standard_bar(pivot_layout, "Pivot Slide TabBar", "特点：选中时会有一个滑动动画效果。", 3)
    add_standard_bar(pivot_layout, "Pivot Stretch TabBar", "特点：选中时会有一个拉伸动画效果。", 4)
    pivot_layout.addStretch()
    main_layout.addWidget(pivot_card, 1)

    # ================= 2. Segmented Tabs =================
    seg_card, seg_layout = create_card_container()
    
    bar_seg_slide = add_standard_bar(seg_layout, "Segmented Slide TabBar", "特点：Segmented风格，选中时会有一个滑动动画效果。", 6)
    bar_seg_fade = add_standard_bar(seg_layout, "Segmented Fade TabBar", "特点：选中时会有一个淡入淡出动画效果。", 7)
    bar_seg_winui3 = add_standard_bar(seg_layout, "Segmented WinUI3 TabBar", "特点：Segmented风格，WinUI3 的选中指示器效果。", 9)
    
    winui3_icon_bar = QTabBar()
    winui3_icon_bar.setAttribute(Qt.WA_StyledBackground, True)
    winui3_icon_bar.setDrawBase(False)
    winui3_icon_bar.setExpanding(False)
    winui3_icon_bar.setProperty("tabBarStyle", 9)
    for _ in range(5): winui3_icon_bar.addTab("")
    seg_layout.addWidget(winui3_icon_bar)

    add_section_title(seg_layout, "Segmented Gallery Style", "特点：半圆胶囊 + 自定义背景/选中/悬停/按下色")
    
    default_gallery_bar = QTabBar()
    default_gallery_bar.setAttribute(Qt.WA_StyledBackground, True)
    default_gallery_bar.setDrawBase(False)
    default_gallery_bar.setProperty("tabBarStyle", 6)
    default_gallery_bar.setProperty("segmentedSemiRound", True)
    default_gallery_bar.setProperty("segmentedBackgroundColor", QColor("#D9D9DD"))
    default_gallery_bar.setProperty("segmentedBackgroundColorDark", QColor("#3F3F46"))
    default_gallery_bar.setProperty("segmentedSelectedColor", QColor("#FFFFFF"))
    default_gallery_bar.setProperty("segmentedSelectedColorDark", QColor("#5C5C64"))
    default_gallery_bar.setProperty("segmentedHoverColor", QColor("#E6E6EA"))
    default_gallery_bar.setProperty("segmentedHoverColorDark", QColor("#4A4A52"))
    default_gallery_bar.setProperty("segmentedPressedColor", QColor("#D0D0D4"))
    default_gallery_bar.setProperty("segmentedPressedColorDark", QColor("#55555D"))
    default_gallery_bar.addTab("Weekly")
    default_gallery_bar.addTab("Daily")
    default_gallery_bar.addTab("Monthly")
    default_gallery_bar.setCurrentIndex(1)
    default_gallery_bar.setExpanding(True)
    default_gallery_bar.setMaximumWidth(150 * default_gallery_bar.count())
    seg_layout.addWidget(default_gallery_bar)

    purple_gallery_bar = QTabBar()
    purple_gallery_bar.setAttribute(Qt.WA_StyledBackground, True)
    purple_gallery_bar.setDrawBase(False)
    purple_gallery_bar.setProperty("tabBarStyle", 6)
    purple_gallery_bar.setProperty("segmentedSemiRound", True)
    purple_gallery_bar.setProperty("segmentedBackgroundColor", QColor("#D9D9DD"))
    purple_gallery_bar.setProperty("segmentedBackgroundColorDark", QColor("#3F3F46"))
    purple_gallery_bar.setProperty("segmentedSelectedColor", QColor("#7E57E8"))
    purple_gallery_bar.setProperty("segmentedSelectedColorDark", QColor("#6E4FD6"))
    purple_gallery_bar.setProperty("segmentedHoverColor", QColor("#E6E6EA"))
    purple_gallery_bar.setProperty("segmentedHoverColorDark", QColor("#4A4A52"))
    purple_gallery_bar.setProperty("segmentedPressedColor", QColor("#D0D0D4"))
    purple_gallery_bar.setProperty("segmentedPressedColorDark", QColor("#55555D"))
    purple_gallery_bar.addTab("Overview")
    purple_gallery_bar.addTab("Stats")
    purple_gallery_bar.addTab("Goals")
    purple_gallery_bar.addTab("History")
    purple_gallery_bar.setCurrentIndex(0)
    purple_gallery_bar.setExpanding(True)
    purple_gallery_bar.setMaximumWidth(150 * purple_gallery_bar.count())
    seg_layout.addWidget(purple_gallery_bar)

    icononly_gallery_bar = QTabBar()
    icononly_gallery_bar.setAttribute(Qt.WA_StyledBackground, True)
    icononly_gallery_bar.setDrawBase(False)
    icononly_gallery_bar.setProperty("tabBarStyle", 6)
    icononly_gallery_bar.setProperty("segmentedSemiRound", True)
    icononly_gallery_bar.setProperty("segmentedBackgroundColor", QColor("#D9D9DD"))
    icononly_gallery_bar.setProperty("segmentedBackgroundColorDark", QColor("#3F3F46"))
    icononly_gallery_bar.setProperty("segmentedSelectedColor", QColor("#FFFFFF"))
    icononly_gallery_bar.setProperty("segmentedSelectedColorDark", QColor("#5C5C64"))
    icononly_gallery_bar.setProperty("segmentedHoverColor", QColor("#E6E6EA"))
    icononly_gallery_bar.setProperty("segmentedHoverColorDark", QColor("#4A4A52"))
    icononly_gallery_bar.setProperty("segmentedPressedColor", QColor("#D0D0D4"))
    icononly_gallery_bar.setProperty("segmentedPressedColorDark", QColor("#55555D"))
    for _ in range(4): icononly_gallery_bar.addTab("")
    icononly_gallery_bar.setCurrentIndex(1)
    icononly_gallery_bar.setExpanding(False)
    icononly_gallery_bar.setMaximumWidth(56 * icononly_gallery_bar.count())
    seg_layout.addWidget(icononly_gallery_bar)

    seg_layout.addStretch()
    main_layout.addWidget(seg_card, 1)

    # ================= 3. Pill Tabs =================
    pill_card, pill_layout = create_card_container()
    add_section_title(pill_layout, "Pill TabBar")
    pill_bar = QTabBar()
    pill_bar.setTabsClosable(True)
    pill_bar.setExpanding(False)
    pill_bar.setProperty("tabBarStyle", 5)
    pill_bar.addTab("Home")
    pill_bar.addTab("Search")
    pill_bar.addTab("Settings")
    pill_bar.addTab("Help")
    pill_bar.addTab("About")
    pill_layout.addWidget(pill_bar)
    pill_layout.addStretch()
    main_layout.addWidget(pill_card, 1)

    # ================= 4. Capsule Tabs =================
    cap_card, cap_layout = create_card_container()
    add_section_title(cap_layout, "Capsule TabBar", "特点：浏览器标签样式。")
    cap_tab_widget = QTabWidget()
    cap_tab_widget.setMinimumHeight(200)
    cap_tab_widget.setTabsClosable(True)
    cap_tab_widget.setMovable(True)
    cap_bar = cap_tab_widget.tabBar()
    cap_bar.setProperty("tabBarStyle", 1)
    
    colors = ["#FFE4E1", "#E0FFFF", "#F0FFF0", "#FFFACD", "#E6E6FA"]
    names = ["Home", "Search", "Settings", "Help", "About"]
    for i in range(5):
        w = QWidget()
        w.setStyleSheet(f"background-color: {colors[i]}; border-radius: 8px;")
        cap_tab_widget.addTab(w, names[i])
    cap_layout.addWidget(cap_tab_widget)
    main_layout.addWidget(cap_card, 1)
    
    # ================= 5. Navigation Tabs =================
    nav_card, nav_layout = create_card_container()
    add_section_title(nav_layout, "Navigation TabBar", "特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果")
    
    nav_tab_widget = QTabWidget()
    nav_tab_widget.setTabPosition(QTabWidget.West)
    nav_tab_widget.setMinimumHeight(300)
    
    nav_bar = nav_tab_widget.tabBar()
    nav_bar.setShape(QTabBar.RoundedWest)
    nav_bar.setDrawBase(False)
    nav_bar.setExpanding(False)
    nav_bar.setProperty("TextAlign", int(Qt.AlignVCenter | Qt.AlignLeft))
    nav_bar.setProperty("tabBarStyle", 8)
    
    nav_names = ["Overview", "Files", "History", "Insights", "Settings"]
    nav_full_names = ["Overview Page", "Files Page", "History Page", "Insights Page", "Settings Page"]
    nav_colors = ["#F4F8FF", "#F0FBF6", "#FFF8EE", "#F8F3FF", "#F5F5F5"]
    
    for i in range(5):
        page = QLabel(nav_full_names[i])
        page.setAlignment(Qt.AlignCenter)
        page.setMinimumHeight(220)
        page.setStyleSheet(f"background-color: {nav_colors[i]}; border-radius: 8px; font-size: 24px;")
        nav_tab_widget.addTab(page, nav_names[i])
        
    nav_layout.addWidget(nav_tab_widget, 1)
    main_layout.addWidget(nav_card, 1)
    
    main_layout.addStretch()

    # Track icons
    def set_tab_icon(bar, index, code):
        bar.setTabIcon(index, create_fluent_icon(code))
        # Add a custom attribute to track the icon code for theme changes
        if not hasattr(bar, '_icon_map'):
            bar._icon_map = {}
        bar._icon_map[index] = code
        main_controller.tabbar_icon_map[bar] = bar._icon_map

    # Assign icons
    icon_codes = ["\ue80f", "\ue721", "\ue713", "\ue897", "\ue946"]
    
    bars = [bar_seg_slide, bar_seg_fade, bar_seg_winui3, pill_bar, cap_bar, nav_bar]
    for b in bars:
        for i in range(5):
            set_tab_icon(b, i, icon_codes[i])

    for i in range(5):
        set_tab_icon(winui3_icon_bar, i, icon_codes[i])
        
    for i in range(4):
        set_tab_icon(icononly_gallery_bar, i, icon_codes[i])
