from PySide6.QtWidgets import QTableWidgetItem
from PySide6.QtCore import Qt
from utils import query_installed_software

def setup_table_widget(window):
    table = window.tableWidget
    table.clear()
    table.setColumnCount(5)
    table.setHorizontalHeaderLabels(["软件名称", "版本", "发布商", "安装日期", "来源"])
    table.verticalHeader().setMinimumSectionSize(50)
    table.verticalHeader().setDefaultSectionSize(50)
    table.horizontalHeader().setStretchLastSection(True)
    table.horizontalHeader().setDefaultAlignment(Qt.AlignLeft | Qt.AlignVCenter)
    table.horizontalHeader().setFixedHeight(50)
    table.verticalHeader().setVisible(False)
    table.setAlternatingRowColors(True)
    table.setShowGrid(False)
    
    h_font = table.horizontalHeader().font()
    h_font.setPixelSize(14)
    table.horizontalHeader().setFont(h_font)
    
    software_data = query_installed_software()
    if not software_data:
        table.setRowCount(1)
        table.setItem(0, 0, QTableWidgetItem("未读取到安装软件信息"))
    else:
        table.setRowCount(len(software_data))
        for row, (name, ver, pub, date, src) in enumerate(software_data):
            table.setItem(row, 0, QTableWidgetItem(name))
            table.setItem(row, 1, QTableWidgetItem(ver))
            table.setItem(row, 2, QTableWidgetItem(pub))
            table.setItem(row, 3, QTableWidgetItem(date))
            table.setItem(row, 4, QTableWidgetItem(src))
        table.resizeColumnsToContents()
        table.selectRow(0)
