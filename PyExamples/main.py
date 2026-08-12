import sys
import os
from PySide6.QtWidgets import QApplication, QStackedWidget
from PySide6.QtCore import QCoreApplication, QFile
from PySide6.QtGui import QFont
from PySide6.QtUiTools import QUiLoader

from mainwindow import MainWindowController
from table_showcase import setup_table_widget
from tab_showcase import setup_tab_showcase
from basic_showcase import setup_basic_showcase

class ExStackedWidget(QStackedWidget):
    pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    app.setProperty("_q_scrollHint_center", False)
    app.setProperty("_q_themestyle", 0)            
    
    plugin_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plugins")
    QCoreApplication.addLibraryPath(plugin_path)
    app.setStyle("FluentUI3")

    font = app.font()
    font.setPixelSize(13)
    font.setFamily("微软雅黑")
    font.setHintingPreference(QFont.PreferNoHinting)
    app.setFont(font)

    ui_path = os.path.abspath(os.path.join(
        os.path.dirname(__file__), 
        "..", "Examples", "Gallery", "mainwindow.ui"
    ))
    
    loader = QUiLoader()
    loader.registerCustomWidget(ExStackedWidget) 
    
    ui_file = QFile(ui_path)
    if not ui_file.open(QFile.ReadOnly):
        print(f"无法打开 UI 文件: {ui_path}\n错误信息: {ui_file.errorString()}")
        sys.exit(-1)
        
    window = loader.load(ui_file)
    ui_file.close()
    
    if not window:
        print(f"UI 文件加载失败: {loader.errorString()}")
        sys.exit(-1)
        
    # 集中处理主窗口绑定 (菜单、工具栏、导航、全局设置)
    controller = MainWindowController(window)
    
    # 初始化各个子模块 UI 逻辑
    setup_table_widget(window)
    setup_tab_showcase(window, controller)
    setup_basic_showcase(window, controller)

    window.setWindowTitle("FluentUI3 PySide6 Gallery Demo")
    window.show()
    
    sys.exit(app.exec())
