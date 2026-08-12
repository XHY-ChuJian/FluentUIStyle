from PySide6.QtWidgets import QMenu, QToolButton
from PySide6.QtGui import QColor, QPalette
from PySide6.QtCore import Qt
from utils import create_fluent_icon

def change_accent_palette(widget, color_hex):
    palette = widget.palette()
    palette.setColor(QPalette.Accent, QColor(color_hex))
    widget.setPalette(palette)

def setup_basic_showcase(window, main_controller):
    # ================= 基础控件 (Basic Controls) 1:1 还原 =================
    
    # 辅助函数：设置图标并加入主题追踪
    def set_button_icon(btn, code):
        btn.setIcon(create_fluent_icon(code))
        main_controller.button_icon_map[btn] = code

    # 工具按钮
    set_button_icon(window.toolButton, "\ue8c3")
    window.pushButton_10.setText("工具按钮")
    set_button_icon(window.pushButton_10, "\ue713")

    # 带菜单的工具按钮
    window.toolButton_3.setAutoRaise(False)
    window.toolButton_3.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
    window.toolButton_3.setPopupMode(QToolButton.InstantPopup)
    window.toolButton_3.setText("菜单按钮")

    menu = QMenu(window.toolButton_3)
    main_controller.add_action(menu, "\ue8a5", "新建文件")
    main_controller.add_action(menu, "\ue8b5", "新建项目")
    main_controller.add_action(menu, "\ue8c3", "最近打开")
    main_controller.add_action(menu, "\ue8a5", "打开文件")
    
    window.toolButton_3.setMenu(menu)
    window.toolButton_4.setMenu(menu)

    # 包含文本和图标的工具按钮
    window.toolButton_4.setToolButtonStyle(Qt.ToolButtonTextUnderIcon)
    window.toolButton_4.setText("上下按钮")
    set_button_icon(window.toolButton_4, "\uE804")
    
    # 自动弹起的工具按钮
    set_button_icon(window.tBtnAutoRaise, "\ue804")
    
    # 其他属性设置 (无边框和特殊绘图开关)
    window.dial_3.setProperty("dialDrawValue", False)
    window.scrollArea_2.viewport().setAutoFillBackground(False)
    window.scrollAreaWidgetContents.setAutoFillBackground(False)
    window.scrollAreaWidgetContents_4.setAutoFillBackground(False)
    
    # 搜索框占位符与后置图标
    window.lineEditSerach.setPlaceholderText("搜索...")
    window.lineEditSerach.setClearButtonEnabled(True)
    search_action = window.lineEditSerach.addAction(create_fluent_icon("\ue721"), window.lineEditSerach.ActionPosition.TrailingPosition)
    main_controller.action_icon_map[search_action] = "\ue721"

    # 进度条样式: ProgressBarThick = 1
    window.progressBar.setProperty("progressBarStyle", 1)
    
    # SpinBox 按钮布局: ArrowsHorizontalRight = 0
    window.spinBox.setProperty("spinBoxButtonLayout", 0)
    
    window.checkBox_5.setText("Off")
    
    # 树形控件项高度调整
    window.treeWidget.setProperty("ItemHeight", 32)
    window.treeWidget.setIndentation(20)

    # 使用 change_accent_palette 修改各控件的 Accent 颜色
    change_accent_palette(window.checkBox_2, "#4CAF50")
    change_accent_palette(window.checkBox_3, "#FFC107")
    change_accent_palette(window.checkBox_4, "#FF8F00")
    
    change_accent_palette(window.radioButton, "#FFC107")
    change_accent_palette(window.radioButton_2, "#FF8F00")
