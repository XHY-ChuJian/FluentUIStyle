# Wrap menu strings with mw->tr() in mainwindow.cpp buildMainMenus.
import re
from pathlib import Path

path = Path(__file__).resolve().parent.parent / "mainwindow.cpp"
s = path.read_text(encoding="utf-8")
s = s.replace(
    "static void initializeMenu(QMenuBar *menuBar)",
    "static void buildMainMenus(MainWindow *mw, QMenuBar *menuBar)",
)
s = s.replace("initializeMenu(m_menuBar);", "buildMainMenus(this, m_menuBar);")
s = re.sub(r'menuBar->addMenu\("([^"]+)"\)', r'menuBar->addMenu(mw->tr("\1"))', s)
s = re.sub(
    r"addMenu\(createFluentIcon((?:[^()]|\([^)]*\))*)\), \"([^\"]+)\"\)",
    r"addMenu(createFluentIcon\1), mw->tr(\"\2\"))",
    s,
)
s = re.sub(
    r"addAction\(createFluentIcon((?:[^()]|\([^)]*\))*)\), \"([^\"]+)\"\)",
    r"addAction(createFluentIcon\1), mw->tr(\"\2\"))",
    s,
)
s = re.sub(r'zoomMenu->addAction\("([^"]+)"\)', r'zoomMenu->addAction(mw->tr("\1"))', s)
path.write_text(s, encoding="utf-8")
print("OK")
