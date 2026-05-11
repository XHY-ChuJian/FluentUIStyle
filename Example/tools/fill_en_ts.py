# Fill Example_en_US.ts translations (remove unfinished). Run from repo root:
#   python Example/tools/fill_en_ts.py
import re
import xml.etree.ElementTree as ET
from pathlib import Path

TS_PATH = Path(__file__).resolve().parent.parent / "translations" / "Example_en_US.ts"

PAIRS = r"""
文件|File
新建文件|New File
新建项目|New Project
最近打开|Recent
打开文件|Open File
打开项目|Open Project
保存|Save
另存为|Save As
关闭文件|Close File
退出|Exit
编辑|Edit
撤销|Undo
重做|Redo
剪切|Cut
复制|Copy
粘贴|Paste
查找|Find
替换|Replace
高级|Advanced
自动格式化|Auto Format
排序行|Sort Lines
删除空行|Delete Empty Lines
视图|View
显示工具栏|Show Toolbar
显示状态栏|Show Status Bar
显示侧边栏|Show Sidebar
显示输出窗口|Show Output
缩放|Zoom
放大|Zoom In
缩小|Zoom Out
恢复默认|Reset
构建|Build
构建项目|Build Project
重新构建|Rebuild
运行|Run
调试|Debug
构建目标|Build Target
帮助|Help
文档|Documentation
API参考|API Reference
检查更新|Check for Updates
关于|About
界面语言|Display language
简体中文|Simplified Chinese
English|English
跟随系统语言|Follow system language
语言已保存。是否立即重启应用程序？|Your language preference was saved. Restart the application now?
立即重启|Restart now
稍后|Later
无法重新启动应用程序，请手动关闭后再次打开。|Could not restart the application. Please close it and open it again.
FluentUI Demo - QStyle [Qt-Version %1]|FluentUI Demo - QStyle [Qt-Version %1]
搜索...|Search...
Off|Off
On|On
工具栏|Toolbar
新建|New
打开|Open
禁用|Disabled
主题：|Theme:
配色：|Color scheme:
样式：|Style:
窗口背景：|Window background:
无|None
基础控件|Basic controls
表格控件|Table controls
列表控件|List controls
树形控件|Tree controls
导航控件|Navigation controls
图标库|Icon gallery
设置|Settings
测试节点|Test node
子节点%1|Child %1
子节点%1-%2|Child %1-%2
软件名称|Name
版本|Version
发布商|Publisher
安装日期|Install date
来源|Source
未读取到安装软件信息|No installed software information was read
工具按钮|Tool button
上下按钮|Up/Down button
菜单按钮|Menu button
子窗口 %1|Sub-window %1
切换视图模式|Toggle view mode
无法打开changelog.txt, %1|Could not open changelog.txt: %1
恢复默认|Restore default
新建项目|New Project
常用基础控件展示|Common basic controls
TableView 示例|TableView sample
新建行|New row
新建列|New column
ListView 示例|ListView sample
TreeView 示例|TreeView sample
省份|Province
四川|Sichuan
成都|Chengdu
绵阳|Mianyang
德阳|Deyang
资阳|Ziyang
遂宁|Suining
云南|Yunnan
贵州|Guizhou
新建子项目|New sub-item
主题模式|Theme mode
窗口背景|Window background
图片|Picture
正常|Normal
导航模式|Navigation mode
强调色|Accent color
浅色|Light
暗色|Dark
日志 And 提示|Log and tips
两个黄鹂鸣翠柳|Two orioles sing among the willows
一行白鹭上青天|A line of egrets ascends the blue sky
窗含西岭千秋雪|My window frames the snow on western peaks
门泊东吴万里船|At the door boats from eastern Wu are moored
君不见黄河之水天上来|The Yellow River pours from the sky
君不见黄河之水天上来，奔流到海不复回。|See how the Yellow River's waters move out of heaven.
君不见高堂明镜悲白发，朝如青丝暮成雪。|See how mirror bright high hall lament white hair at dawn like silk, by dusk turned snow.
人生得意须尽欢，莫使金樽空对月。|When joy arrives, seize it; let not your golden cup face the moon in vain.
天生我材必有用，千金散尽还复来。|Heaven gave me talent for a use; spend a thousand gold — it will return.
烹羊宰牛且为乐，会须一饮三百杯。|Roast the sheep, butcher the ox — be merry; we must drink three hundred cups at one sitting.
岑夫子，丹丘生，将进酒，杯莫停。|Cen master, Danqiu born — bring the wine; let no cup rest.
与君歌一曲，请君为我倾耳听。|I'll sing you a song; lend me your ears.
钟鼓馔玉不足贵，但愿长醉不愿醒。|Bells and drums, jade feasts are not precious — I only wish to stay drunk and never wake.
古来圣贤皆寂寞，惟有饮者留其名。|Since olden days the sages have been lonely; only drinkers leave their names.
陈王昔时宴平乐，斗酒十千恣欢谑。|Prince Chen once feasted in Pingyue — ten thousand coins for a cask of wine, wild mirth.
主人何为言少钱，径须沽取对君酌。|Host, why say you lack silver? Go buy more wine — we'll drink together.
五花马，千金裘，呼儿将出换美酒，与尔同销万古愁|Dappled steed, fur worth a thousand — call the boy to barter for fine wine, and drown with you ten thousand years of sorrow
人生若只如初见，何事秋风悲画扇|If life could stay as first sight, why should the autumn wind grieve the painted fan?
界面语言|Display language
TabBar多种样式示例|Tab bar style samples
Pivot Grow TabBar|Pivot Grow TabBar
Pivot Slide TabBar|Pivot Slide TabBar
Pivot Stretch TabBar|Pivot Stretch TabBar
特点：选中时会有一个生长动画效果。|Selected tab grows with an animation.
特点：选中时会有一个滑动动画效果。|Selected tab slides with an animation.
特点：选中时会有一个拉伸动画效果。|Selected tab stretches with an animation.
Segmented Slide TabBar|Segmented Slide TabBar
Segmented Fade TabBar|Segmented Fade TabBar
Segmented WinUI3 TabBar|Segmented WinUI3 TabBar
特点：Segmented风格，选中时会有一个滑动动画效果。|Segmented style; selected tab slides with an animation.
特点：选中时会有一个淡入淡出动画效果。|Fade in/out animation on selection.
特点：Segmented风格，WinUI3 的选中指示器效果。|Segmented style with WinUI3 selection indicator.
Segmented Gallery Style|Segmented gallery style
特点：半圆胶囊 + 自定义背景/选中/悬停/按下色|Semi-round pills with custom background/selected/hover/pressed colors.
Weekly|Weekly
Daily|Daily
Monthly|Monthly
Overview|Overview
Stats|Stats
Goals|Goals
History|History
Pill TabBar|Pill TabBar
Capsule TabBar|Capsule TabBar
特点：浏览器标签样式。|Browser-style tabs.
Home Page|Home Page
Search Page|Search Page
Settings Page|Settings Page
Help Page|Help Page
About Page|About Page
Navigation TabBar|Navigation TabBar
特点：适合用于侧边栏的导航菜单，选项卡垂直排列，选中时指示器有个变长效果|For side navigation: vertical tabs with a variable-length selection indicator.
Overview Page|Overview Page
Files Page|Files Page
History Page|History Page
Insights Page|Insights Page
Settings Page|Settings Page
Files|Files
History|History
Insights|Insights
Home|Home
Search|Search
Settings|Settings
Help|Help
About|About
""".strip()

TABLE = {}
for line in PAIRS.splitlines():
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    if "|" not in line:
        continue
    k, v = line.split("|", 1)
    TABLE[k.strip()] = v.strip()


def source_text(msg: ET.Element) -> str:
    el = msg.find("source")
    if el is None:
        return ""
    return "".join(el.itertext())


def translate(src: str) -> str:
    if src in TABLE:
        return TABLE[src]
    if not re.search(r"[\u4e00-\u9fff\u3000-\u303f]", src):
        return src
    return src


def main() -> None:
    tree = ET.parse(TS_PATH)
    root = tree.getroot()
    for msg in root.iter("message"):
        src = source_text(msg)
        tr_el = msg.find("translation")
        if tr_el is None:
            continue
        tr_el.text = translate(src)
        if "type" in tr_el.attrib:
            del tr_el.attrib["type"]
    tree.write(TS_PATH, encoding="utf-8", xml_declaration=True)


if __name__ == "__main__":
    main()
    print("Wrote", TS_PATH)
