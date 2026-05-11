import re
from pathlib import Path

p = Path(__file__).resolve().parent.parent / "mainwindow.cpp"
s = p.read_text(encoding="utf-8")
s2 = re.sub(
    r'addAction\(createFluentIcon\(([^)]+)\), "([^"]+)"\)',
    r'addAction(createFluentIcon(\1), mw->tr("\2"))',
    s,
)
s2 = re.sub(
    r'addMenu\(createFluentIcon\(([^)]+)\), "([^"]+)"\)',
    r'addMenu(createFluentIcon(\1), mw->tr("\2"))',
    s2,
)
p.write_text(s2, encoding="utf-8")
print("done")
