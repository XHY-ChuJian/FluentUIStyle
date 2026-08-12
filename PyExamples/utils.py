import winreg
from PySide6.QtGui import QIcon, QPixmap, QPainter, QFont
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication

def create_fluent_icon(icon_code):
    pixmap = QPixmap(30, 30)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing | QPainter.SmoothPixmapTransform)
    font = QFont("Segoe Fluent Icons")
    font.setPixelSize(25)
    painter.setFont(font)
    is_dark = QApplication.instance().property("_q_colorscheme") == 1
    painter.setPen(Qt.white if is_dark else Qt.black)
    painter.drawText(pixmap.rect(), Qt.AlignCenter, icon_code)
    painter.end()
    return QIcon(pixmap)

def query_installed_software():
    software_list = []
    dedupe_keys = set()

    def append_registry(hive, subkey_path, source_label):
        try:
            key = winreg.OpenKey(hive, subkey_path, 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY)
        except OSError:
            return
        num_subkeys = winreg.QueryInfoKey(key)[0]
        for i in range(num_subkeys):
            try:
                subkey_name = winreg.EnumKey(key, i)
                with winreg.OpenKey(key, subkey_name) as subkey:
                    def get_val(name, default=""):
                        try:
                            return str(winreg.QueryValueEx(subkey, name)[0]).strip()
                        except OSError:
                            return default
                    display_name = get_val("DisplayName")
                    if not display_name: continue
                    try:
                        system_comp = winreg.QueryValueEx(subkey, "SystemComponent")[0]
                        if system_comp == 1: continue
                    except OSError:
                        pass
                    if get_val("ParentKeyName"): continue
                    release_type = get_val("ReleaseType").lower()
                    if "update" in release_type or "hotfix" in release_type: continue
                    version = get_val("DisplayVersion", "-")
                    publisher = get_val("Publisher", "-")
                    install_date = get_val("InstallDate", "-")
                    if len(install_date) == 8 and install_date.isdigit():
                        install_date = f"{install_date[:4]}-{install_date[4:6]}-{install_date[6:]}"
                    dedupe_key = f"{display_name}|{version}|{publisher}"
                    if dedupe_key in dedupe_keys: continue
                    dedupe_keys.add(dedupe_key)
                    software_list.append((display_name, version, publisher, install_date, source_label))
            except OSError:
                continue

    append_registry(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall", "HKLM 64-bit")
    append_registry(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall", "HKLM 32-bit")
    append_registry(winreg.HKEY_CURRENT_USER, r"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall", "HKCU")
    software_list.sort(key=lambda x: x[0].lower())
    return software_list
