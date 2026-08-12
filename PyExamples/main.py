import sys
import os
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton, QCheckBox, QProgressBar, QTabWidget
from PySide6.QtCore import QCoreApplication

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # 动态加载当前文件所在目录下的 plugins 文件夹，以寻找 Qt Style 插件
    plugin_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "plugins")
    QCoreApplication.addLibraryPath(plugin_path)
    
    # 应用 FluentUI3 样式 (如果 plugins/styles/ 下没有对应的 dll 插件，这句将无效并使用系统默认样式)
    app.setStyle("FluentUI3")

    window = QWidget()
    window.setWindowTitle("FluentUI3 PySide6 Demo")
    window.resize(400, 300)
    layout = QVBoxLayout(window)

    # 1. 强调色按钮 (Accent Button)
    btn = QPushButton("强调按钮")
    btn.setProperty("accent", True)  # C++: ButtonAccentStyleProperty
    layout.addWidget(btn)

    # 2. 开关按钮 (Switch Button)
    switch_btn = QCheckBox("拨动开关")
    switch_btn.setProperty("isSwitchButton", True)  # C++: SwitchStyleProperty
    layout.addWidget(switch_btn)

    # 3. 环形进度条 (Ring ProgressBar)
    progress = QProgressBar()
    progress.setValue(60)
    # ProgressBarStyle 枚举: Thin=0, Thick=1, Ring=2
    progress.setProperty("progressBarStyle", 2) 
    progress.setProperty("progressBarThickness", 8)
    layout.addWidget(progress)

    # 4. 导航风格选项卡 (TabWidget)
    tabs = QTabWidget()
    tabs.addTab(QWidget(), "Home")
    tabs.addTab(QWidget(), "Settings")
    # TabBarStyle 枚举: Segmented_WinUI3 = 9
    tabs.tabBar().setProperty("tabBarStyle", 9) 
    layout.addWidget(tabs)

    window.show()
    sys.exit(app.exec())
