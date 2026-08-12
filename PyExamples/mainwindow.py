from PySide6.QtGui import QAction, QKeySequence
from PySide6.QtWidgets import QMenu, QToolBar, QCheckBox, QComboBox, QStyleFactory, QTabBar, QLabel, QTreeWidget, QTreeWidgetItem, QApplication
from PySide6.QtCore import Qt
from utils import create_fluent_icon

class MainWindowController:
    def __init__(self, window):
        self.window = window
        self.app = QApplication.instance()
        
        # 1:1 C++ 里的 g_actionIconMap 和 g_menuIconMap
        self.action_icon_map = {}
        self.menu_icon_map = {}
        self.tree_item_icon_map = {}
        self.button_icon_map = {}
        self.tabbar_icon_map = {}
        
        self.setup_menus_and_toolbars()
        self.setup_navigation()
        self.setup_settings()
        
    def add_action(self, parent, icon_code, text):
        action = parent.addAction(create_fluent_icon(icon_code), text)
        self.action_icon_map[action] = icon_code
        return action
        
    def add_menu(self, parent, icon_code, text):
        menu = parent.addMenu(create_fluent_icon(icon_code), text)
        self.menu_icon_map[menu] = icon_code
        return menu

    def update_action_icons(self):
        # 主题切换时，重新生成黑白对应的图标并替换
        for action, code in self.action_icon_map.items():
            action.setIcon(create_fluent_icon(code))
        for menu, code in self.menu_icon_map.items():
            menu.setIcon(create_fluent_icon(code))
        for item, code in self.tree_item_icon_map.items():
            item.setIcon(0, create_fluent_icon(code))
        for btn, code in self.button_icon_map.items():
            btn.setIcon(create_fluent_icon(code))
        for bar, icon_map in self.tabbar_icon_map.items():
            for index, code in icon_map.items():
                bar.setTabIcon(index, create_fluent_icon(code))
        
    def setup_menus_and_toolbars(self):
        menubar = self.window.menuBar()
        # --- File Menu ---
        file_menu = menubar.addMenu("文件")
        a_new_file = self.add_action(file_menu, "\ue8a5", "新建文件")
        a_new_file.setShortcut(QKeySequence("Ctrl+N"))
        self.add_action(file_menu, "\ue8b5", "新建项目")
        
        recent_menu = self.add_menu(file_menu, "\ue8c3", "最近打开")
        recent_menu.addAction("project1")
        recent_menu.addAction("project2")
        recent_menu.addAction("example.cpp")
        
        a_open = self.add_action(file_menu, "\ue8a5", "打开文件")
        a_open.setShortcut(QKeySequence("Ctrl+O"))
        self.add_action(file_menu, "\ue8b5", "打开项目")
        file_menu.addSeparator()
        a_save = self.add_action(file_menu, "\ue74e", "保存")
        a_save.setShortcut(QKeySequence("Ctrl+S"))
        a_save_as = self.add_action(file_menu, "\ue74e", "另存为")
        a_save_as.setShortcut(QKeySequence("Ctrl+Shift+S"))
        file_menu.addSeparator()
        self.add_action(file_menu, "\ue8bb", "关闭文件")
        a_exit = self.add_action(file_menu, "\ue8bb", "退出")
        a_exit.setShortcut(QKeySequence("Ctrl+Q"))
        a_exit.triggered.connect(self.window.close)
        
        # --- Edit Menu ---
        edit_menu = menubar.addMenu("编辑")
        a_undo = self.add_action(edit_menu, "\ue7a7", "撤销")
        a_undo.setShortcut(QKeySequence("Ctrl+Z"))
        a_redo = self.add_action(edit_menu, "\ue7a6", "重做")
        a_redo.setShortcut(QKeySequence("Ctrl+Y"))
        edit_menu.addSeparator()
        a_cut = self.add_action(edit_menu, "\ue8c6", "剪切")
        a_cut.setShortcut(QKeySequence("Ctrl+X"))
        a_copy = self.add_action(edit_menu, "\ue8c8", "复制")
        a_copy.setShortcut(QKeySequence("Ctrl+C"))
        a_paste = self.add_action(edit_menu, "\ue8c7", "粘贴")
        a_paste.setShortcut(QKeySequence("Ctrl+V"))
        edit_menu.addSeparator()
        a_find = self.add_action(edit_menu, "\ue721", "查找")
        a_find.setShortcut(QKeySequence("Ctrl+F"))
        a_replace = self.add_action(edit_menu, "\ue8ac", "替换")
        a_replace.setShortcut(QKeySequence("Ctrl+H"))
        
        advanced_menu = self.add_menu(edit_menu, "\ue713", "高级")
        a_format = self.add_action(advanced_menu, "\ue930", "自动格式化")
        a_format.setCheckable(True)
        self.add_action(advanced_menu, "\ue930", "排序行")
        self.add_action(advanced_menu, "\ue8bb", "删除空行")
        
        # --- View Menu ---
        view_menu = menubar.addMenu("视图")
        a_show_tb = self.add_action(view_menu, "\ue728", "显示工具栏")
        a_show_tb.setCheckable(True)
        a_show_tb.setChecked(True)
        a_show_sb = self.add_action(view_menu, "\ue9d9", "显示状态栏")
        a_show_sb.setCheckable(True)
        a_show_sb.setChecked(True)
        view_menu.addSeparator()
        a_show_sidebar = self.add_action(view_menu, "\ue728", "显示侧边栏")
        a_show_sidebar.setCheckable(True)
        a_show_sidebar.setChecked(True)
        a_show_out = self.add_action(view_menu, "\ue7e8", "显示输出窗口")
        a_show_out.setCheckable(True)
        
        zoom_menu = self.add_menu(view_menu, "\ue71e", "缩放")
        zoom_menu.addAction("放大")
        zoom_menu.addAction("缩小")
        zoom_menu.addAction("恢复默认")
        
        # --- Build Menu ---
        build_menu = menubar.addMenu("构建")
        a_build = self.add_action(build_menu, "\ue7b8", "构建项目")
        a_build.setShortcut(QKeySequence("Ctrl+B"))
        self.add_action(build_menu, "\ue7b8", "重新构建")
        build_menu.addSeparator()
        self.add_action(build_menu, "\ue768", "运行")
        self.add_action(build_menu, "\ue7a6", "调试")
        build_target = self.add_menu(build_menu, "\ue8b5", "构建目标")
        build_target.addAction("Debug")
        build_target.addAction("Release")
        
        # --- Help Menu ---
        help_menu = menubar.addMenu("帮助")
        self.add_action(help_menu, "\ue8a5", "文档")
        self.add_action(help_menu, "\ue8a5", "API参考")
        help_menu.addSeparator()
        self.add_action(help_menu, "\ue7b8", "检查更新")
        help_menu.addSeparator()
        self.add_action(help_menu, "\ue946", "关于")
        
        # --- ToolBar Setup ---
        toolbar = QToolBar("工具栏")
        self.window.addToolBar(toolbar)
        toolbar.setAttribute(Qt.WA_TranslucentBackground, True)
        toolbar.setAttribute(Qt.WA_StyledBackground, False)
        
        self.nav_toggle = self.add_action(toolbar, "\uE700", "")
        toolbar.addSeparator()
        
        a_tb_new = self.add_action(toolbar, "\ue8a5", "新建")
        a_tb_new.setShortcut(QKeySequence("Ctrl+N"))
        a_tb_open = self.add_action(toolbar, "\ue8a5", "打开")
        a_tb_open.setShortcut(QKeySequence("Ctrl+O"))
        a_tb_save = self.add_action(toolbar, "\ue74e", "保存")
        a_tb_save.setShortcut(QKeySequence("Ctrl+S"))
        toolbar.addSeparator()
        
        a_tb_undo = self.add_action(toolbar, "\ue7a7", "撤销")
        a_tb_undo.setShortcut(QKeySequence("Ctrl+Z"))
        a_tb_redo = self.add_action(toolbar, "\ue7a6", "重做")
        a_tb_redo.setShortcut(QKeySequence("Ctrl+Y"))
        toolbar.addSeparator()
        
        a_tb_cut = self.add_action(toolbar, "\ue8c6", "剪切")
        a_tb_cut.setShortcut(QKeySequence("Ctrl+X"))
        a_tb_copy = self.add_action(toolbar, "\ue8c8", "复制")
        a_tb_copy.setShortcut(QKeySequence("Ctrl+C"))
        a_tb_paste = self.add_action(toolbar, "\ue8c7", "粘贴")
        a_tb_paste.setShortcut(QKeySequence("Ctrl+V"))
        toolbar.addSeparator()
        
        a_tb_build = self.add_action(toolbar, "\ue7b8", "构建")
        a_tb_build.setShortcut(QKeySequence("Ctrl+B"))
        self.add_action(toolbar, "\ue7b8", "重新构建")
        self.add_action(toolbar, "\ue768", "运行")
        toolbar.addSeparator()
        
        # Disable switch
        disable_cb = QCheckBox("禁用")
        disable_cb.setProperty("isSwitchButton", True)
        disable_cb.clicked.connect(lambda checked: self.window.centralWidget().setEnabled(not checked))
        toolbar.addWidget(disable_cb)
        toolbar.addSeparator()
        
        # Theme combo
        toolbar.addWidget(QLabel("主题："))
        self.theme_combo = QComboBox()
        self.theme_combo.addItems(["浅色", "暗色"])
        self.theme_combo.setCurrentIndex(1 if self.app.property("_q_colorscheme") == 1 else 0)
        toolbar.addWidget(self.theme_combo)
        toolbar.addSeparator()
        
        # Color scheme combo
        toolbar.addWidget(QLabel("配色："))
        self.scheme_combo = QComboBox()
        self.scheme_combo.addItems(["Fluent", "Teams"])
        toolbar.addWidget(self.scheme_combo)
        toolbar.addSeparator()
        
        # Style combo
        toolbar.addWidget(QLabel("样式："))
        self.style_combo = QComboBox()
        self.style_combo.addItems(QStyleFactory.keys())
        toolbar.addWidget(self.style_combo)
        toolbar.addSeparator()
        
        # Widget Bg Mode
        toolbar.addWidget(QLabel("窗口背景："))
        self.bg_tab = QTabBar()
        self.bg_tab.setProperty("tabBarStyle", 9)
        self.bg_tab.addTab("无")
        self.bg_tab.addTab("图片")
        self.bg_tab.addTab("DWM blur")
        toolbar.addWidget(self.bg_tab)
        
    def setup_navigation(self):
        from PySide6.QtWidgets import QWidget
        self.nav_tree = QTreeWidget()
        self.nav_tree.setHeaderHidden(True)
        self.nav_tree.setIndentation(20)
        self.nav_tree.setFixedWidth(240)
        self.nav_tree.setProperty("ItemHeight", 36)
        self.nav_tree.setProperty("navigationViewIndicator", True)
        
        def add_node(parent, title, icon_code, page_name=None, page_index=None):
            item = QTreeWidgetItem(parent)
            item.setText(0, title)
            if icon_code:
                item.setIcon(0, create_fluent_icon(icon_code))
                self.tree_item_icon_map[item] = icon_code
                
            idx = page_index
            if page_name:
                page = self.window.stackedWidget.findChild(QWidget, page_name)
                if page:
                    idx = self.window.stackedWidget.indexOf(page)
                    
            if idx is not None and idx >= 0:
                item.setData(0, Qt.UserRole, idx)
            return item
            
        # 1. 基础项
        add_node(self.nav_tree, "基础控件", "\uE80F", page_index=0)
        add_node(self.nav_tree, "表格控件", "\uE99A", page_index=1)
        add_node(self.nav_tree, "列表控件", "\uE71D", page_index=2)
        add_node(self.nav_tree, "树形控件", "\uED28", page_index=3)
        add_node(self.nav_tree, "导航控件", "\uE8B0", page_index=4)
        
        # 2. ExWidgets 节点 (包含多个子项)
        ex_root = add_node(self.nav_tree, "ExWidgets", "\uE8F1", "pageExRangeSlider")
        add_node(ex_root, "ExRangeSlider", None, "pageExRangeSlider")
        add_node(ex_root, "ExBorderBeam", None, "pageExBorderBeam")
        add_node(ex_root, "ExAudioLevelMeter", None, "pageExAudioLevelMeter")
        add_node(ex_root, "ExRadialGauge", None, "pageExRadialGauge")
        add_node(ex_root, "ExLiquidGauge", None, "pageExLiquidGauge")
        add_node(ex_root, "ExProgressRing", None, "pageExProgressRing")
        add_node(ex_root, "ExTimeline", None, "pageExTimeline")
        add_node(ex_root, "ExColorPicker", "\uE790", "pageExColorPicker")
        add_node(ex_root, "Audiomatic Mini", "\uE8D6", "pageAudiomatic")
        ex_root.setExpanded(True)
        
        # 3. 其他功能菜单
        add_node(self.nav_tree, "Mdi", "\uE9D9", page_index=5)
        add_node(self.nav_tree, "图标库", "\uE8FD", "pageIcons")
        add_node(self.nav_tree, "对话框", "\uE8F2", "pageDialogs")
        
        # 4. 测试节点 (用于演示导航控件的多级树形展开)
        test_root = add_node(self.nav_tree, "测试节点", "\uE9F5", page_index=6)
        for i in range(5):
            child = add_node(test_root, f"子节点{i+1}", None, page_index=6)
            for j in range(3):
                add_node(child, f"子节点{i+1}-{j+1}", None, page_index=6)
                
        # 5. 底部系统项目
        add_node(self.nav_tree, "系统监视", "\uE9D2", "pageSystemResources")
        add_node(self.nav_tree, "更新日志", "\uE823", "pageChangelogTimeline")
        add_node(self.nav_tree, "设置", "\uE713", page_index=6)
        add_node(self.nav_tree, "关于", "\uE77B", page_index=8)
            
        def on_nav_clicked(item, column):
            page_index = item.data(0, Qt.UserRole)
            if page_index is not None and page_index >= 0:
                self.window.stackedWidget.setCurrentIndex(page_index)
        self.nav_tree.itemClicked.connect(on_nav_clicked)
        
        self.window.navigationPaneLayout.addWidget(self.nav_tree, 0, 0)
        if self.nav_tree.topLevelItemCount() > 0:
            first_item = self.nav_tree.topLevelItem(0)
            self.nav_tree.setCurrentItem(first_item)
            on_nav_clicked(first_item, 0)
            
        def set_nav_expanded(expanded):
            self.nav_tree.setProperty("isExpanded", expanded)
            self.nav_tree.style().unpolish(self.nav_tree)
            self.nav_tree.style().polish(self.nav_tree)
            self.window.rBOnlyIcon.setChecked(not expanded)
            self.window.rBIconAndText.setChecked(expanded)
        
        self.nav_toggle.triggered.connect(lambda: set_nav_expanded(not self.nav_tree.property("isExpanded")))
        self.window.rBOnlyIcon.clicked.connect(lambda: set_nav_expanded(False))
        self.window.rBIconAndText.clicked.connect(lambda: set_nav_expanded(True))
        
    def setup_settings(self):
        def apply_theme_index(index):
            self.app.setProperty("_q_colorscheme", index)
            self.app.setStyle("FluentUI3")
            self.theme_combo.blockSignals(True)
            self.theme_combo.setCurrentIndex(index)
            self.theme_combo.blockSignals(False)
            self.window.rBDarkTheme.setChecked(index == 1)
            self.window.rBLightTheme.setChecked(index == 0)
            
            # 使用 Windows 专属 API (ctypes) 通知 DWM 切换原生标题栏颜色
            try:
                import ctypes
                hwnd = int(self.window.windowHandle().winId())
                # DWMWA_USE_IMMERSIVE_DARK_MODE 是 20
                value = ctypes.c_int(1 if index == 1 else 0)
                ctypes.windll.dwmapi.DwmSetWindowAttribute(hwnd, 20, ctypes.byref(value), ctypes.sizeof(value))
            except Exception as e:
                print("无法切换原生标题栏深色模式:", e)
            
            # 关键：主题切换后通知系统重绘所有黑白图标
            self.update_action_icons()
            
        self.theme_combo.currentIndexChanged.connect(apply_theme_index)
        self.window.rBLightTheme.clicked.connect(lambda: apply_theme_index(0))
        self.window.rBDarkTheme.clicked.connect(lambda: apply_theme_index(1))
        
        def apply_scheme_index(index):
            self.app.setProperty("_q_themestyle", index)
            self.app.setStyle("FluentUI3")
        self.scheme_combo.currentIndexChanged.connect(apply_scheme_index)
        
        self.style_combo.currentIndexChanged.connect(lambda: self.app.setStyle(self.style_combo.currentText()))
        
        def apply_widget_bg_mode(index):
            self.app.setProperty("_q_widget_mode", index)
            self.app.setStyle("FluentUI3")
            self.window.rBWidgtModeNormal.setChecked(index == 0)
            self.window.rBWidgetModePixmap.setChecked(index == 1)
            self.window.rBWidgetModeDwmBlur.setChecked(index == 2)
            
        self.bg_tab.currentChanged.connect(apply_widget_bg_mode)
        self.window.rBWidgtModeNormal.clicked.connect(lambda: self.bg_tab.setCurrentIndex(0))
        self.window.rBWidgetModePixmap.clicked.connect(lambda: self.bg_tab.setCurrentIndex(1))
        self.window.rBWidgetModeDwmBlur.clicked.connect(lambda: self.bg_tab.setCurrentIndex(2))
        
        if self.app.property("_q_colorscheme") == 1: self.window.rBDarkTheme.setChecked(True)
        else: self.window.rBLightTheme.setChecked(True)

        def setup_accent_color_widget():
            from PySide6.QtWidgets import QHBoxLayout, QPushButton, QButtonGroup, QSpacerItem, QSizePolicy, QWidget
            from PySide6.QtGui import QColor, QFont
            
            if self.window.widgetAccentColor.layout():
                # PySide6 删除 layout 的一个小技巧：把它交给一个临时控件销毁
                QWidget().setLayout(self.window.widgetAccentColor.layout())
            
            layout = QHBoxLayout(self.window.widgetAccentColor)
            colors = [
                None,              # 恢复默认
                QColor("#FFB900"), # 黄色
                QColor("#FF8C00"), # 橙色
                QColor("#E81123"), # 红色
                QColor("#E3008C"), # 洋红
                QColor("#881798"), # 紫色
                QColor("#0078D4"), # 蓝色
                QColor("#00B7C3"), # 蓝绿
                QColor("#107C10")  # 绿色
            ]
            
            icon_font = QFont("Segoe Fluent Icons")
            icon_font.setPixelSize(20)
            
            btn_group = QButtonGroup(self.window)
            btn_group.setExclusive(True)
            
            for i, color in enumerate(colors):
                btn = QPushButton(self.window.widgetAccentColor)
                btn.setFixedSize(40, 40)
                btn.setCheckable(True)
                btn.setFont(icon_font)
                btn_group.addButton(btn, i)
                
                bg_color = color if color else QColor("#0078D4")
                style = f"""
                    QPushButton {{
                        background-color: {bg_color.name()};
                        border: 1px solid rgba(0, 0, 0, 0.1);
                        border-radius: 4px;
                        color: white;
                    }}
                    QPushButton:hover {{
                        background-color: {bg_color.lighter(110).name()};
                    }}
                    QPushButton:pressed {{
                        background-color: {bg_color.darker(110).name()};
                    }}
                """
                btn.setStyleSheet(style)
                btn.setToolTip("恢复默认" if not color else color.name())
                layout.addWidget(btn)
                
            layout.addSpacerItem(QSpacerItem(1, 1, QSizePolicy.Expanding, QSizePolicy.Preferred))
            
            def on_color_clicked(btn_id):
                for b in btn_group.buttons():
                    b.setText("")
                    
                clicked_btn = btn_group.button(btn_id)
                if clicked_btn:
                    clicked_btn.setText("\uE73E")
                    
                color = colors[btn_id]
                if color:
                    self.app.setProperty("_q_accent_color", color)
                else:
                    self.app.setProperty("_q_accent_color", None)
                    
                # 刷新整个应用的 Fluent 样式
                self.app.setStyle("FluentUI3")
                
            btn_group.idClicked.connect(on_color_clicked)
            
            default_btn = btn_group.button(0)
            if default_btn:
                default_btn.setChecked(True)
                default_btn.setText("\uE73E")
                
        setup_accent_color_widget()
