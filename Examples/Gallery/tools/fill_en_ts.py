# Fill Gallery_en_US.ts translations (remove unfinished). Run from repo root:
#   python examples/Gallery/tools/fill_en_ts.py
import re
import xml.etree.ElementTree as ET
from pathlib import Path

TS_PATH = Path(__file__).resolve().parent.parent / "translations" / "Gallery_en_US.ts"

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
对话框|Dialogs
常用对话框（Qt Widgets）|Common dialogs (Qt Widgets)
以下按钮会弹出模态或非模态对话框，用于在 Fluent 样式下查看常见 Qt 对话框外观。|Buttons below open modal dialogs to preview common Qt dialogs under the Fluent style.
消息框 (QMessageBox)|(QMessageBox)
信息|Information
警告|Warning
严重|Critical
询问|Question
这是一条信息消息。|This is an information message.
这是一条警告消息。|This is a warning message.
这是一条严重错误消息。|This is a critical message.
是否继续？|Do you want to continue?
是否保存文件…|Save before closing…
文档已修改，是否在关闭前保存？|The document has been modified. Save changes before closing?
输入框 (QInputDialog)|(QInputDialog)
单行文本|Single-line text
整数|Integer
浮点数|Decimal
列表选择|Pick from list
多行文本|Multi-line text
输入文本|Enter text
请输入内容：|Please enter text:
示例|Sample text
输入整数|Enter integer
数值：|Value:
输入浮点数|Enter decimal
选择一项|Pick one item
请选择：|Choose:
选项 A|Option A
选项 B|Option B
选项 C|Option C
多行输入|Multi-line input
内容：|Content:
第一行\n第二行|Line 1\nLine 2
颜色与字体|Color and font
选择颜色…|Choose color…
选择颜色|Choose color
选择字体…|Choose font…
选择字体|Choose font
文件对话框 (QFileDialog)|(QFileDialog)
打开文件…|Open file…
打开文件|Open file
保存文件…|Save file…
保存文件|Save file
选择文件夹…|Choose folder…
选择文件夹|Choose folder
进度对话框 (QProgressDialog)|(QProgressDialog)
短时进度…|Short progress…
正在处理…|Working…
自定义对话框 (QDialog + QDialogButtonBox)|(QDialog + QDialogButtonBox)
打开示例对话框…|Open sample dialog…
示例对话框|Sample dialog
这是一个带 QDialogButtonBox 的简单对话框。|This is a simple dialog with a QDialogButtonBox.
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
进度环|Progress ring
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
ProgressRing|ProgressRing
使用标准 QProgressBar 的范围、数值和文本能力，仅通过 progressBarStyle 属性切换为环形外观。|Uses the standard QProgressBar range, value, and text API, switching only its appearance through the progressBarStyle property.
基本状态|Basic states
不确定|Indeterminate
属性|Properties
进度|Progress
不确定动画周期|Indeterminate animation period
环与 Track 宽度|Ring and track thickness
进度环颜色|Progress ring color
Track 颜色|Track color
不确定进度|Indeterminate progress
显示百分比|Show percentage
禁用状态|Disabled state
恢复默认属性|Reset properties
ExLiquidGauge|ExLiquidGauge
参考 Ant Design Charts Liquid 的水波图控件，继承 QProgressBar，并提供圆形、矩形、水滴和三角形裁剪、双层水波与中心文本。|An Ant Design Charts Liquid-inspired widget derived from QProgressBar, with circle, rectangle, pin, and triangle clipping, layered waves, and centered text.
内置形状|Built-in shapes
圆形|Circle
矩形|Rectangle
水滴|Pin
三角形|Triangle
外观|Appearance
播放水波动画|Animate waves
显示中心文本|Show centered text
形状|Shape
文本格式|Text format
文本字号|Text font size
波幅|Wave amplitude
波形数量|Wave count
动画周期|Animation period
后层水波透明度|Rear wave opacity
轮廓宽度|Outline width
轮廓间距|Outline distance
水波颜色|Wave color
背景颜色|Background color
轮廓颜色|Outline color
液面上文字颜色|Text color above liquid
液面下文字颜色|Text color below liquid
ExRadialGauge|ExRadialGauge
基于 QDial 的径向仪表盘，保留范围、数值和交互能力；同一控件可组合 Track、Progress、Ranges 刻度环、数字标签和不同指针样式。|A QDial-based radial gauge that retains range, value, and interaction support while combining Track, Progress, or Ranges scales with labels and different needle styles.
实时属性|Live properties
同一控件的三种配置|Three configurations of the same control
经典指针|Classic needle
进度指针|Progress needle
彩色区间|Colored ranges
公共数值|Shared value
基础|Basic
刻度与标签|Scale and labels
指针与文本|Needle and text
Scale 模式|Scale mode
Track（纯轨道）|Track (track only)
Progress（数值进度）|Progress (value progress)
Ranges（彩色区间）|Ranges (colored ranges)
允许鼠标、键盘和滚轮交互|Allow mouse, keyboard, and wheel interaction
显示数值|Show value
数值|Value
数值动画时长|Value animation duration
数值位置|Value position
中心|Center
底部|Bottom
标题|Title
单位|Unit
数值字号|Value font size
数值颜色|Value color
刻度间隔|Tick spacing
主刻度数量|Major tick count
每段次刻度数量|Minor ticks per interval
次刻度长度|Minor tick length
次刻度宽度|Minor tick width
主刻度长度|Major tick length
主刻度宽度|Major tick width
刻度环宽度|Scale width
起始角度|Minimum angle
结束角度|Maximum angle
指针宽度|Needle width
指针长度比例|Needle length ratio
刻度长度|Tick length
刻度宽度|Tick width
外圈边距|Scale padding
刻度边距|Tick padding
刻线颜色|Tick color
显示刻度数值|Show scale labels
标签间隔|Label spacing
标签边距|Label padding
标签字号|Label font size
标签颜色|Label color
Track 端点|Track cap
环端点|Ring cap
平直|Flat
方形|Square
圆形|Round
指针样式|Needle style
无指针|No needle
线形指针|Line needle
三角指针|Triangle needle
指针颜色|Needle color
显示指针轴心|Show needle hub
轴心半径|Hub radius
区间 1 起点|Range 1 start
区间 1 终点|Range 1 end
区间 1 颜色|Range 1 color
区间 2 起点|Range 2 start
区间 2 终点|Range 2 end
区间 2 颜色|Range 2 color
区间 3 起点|Range 3 start
区间 3 终点|Range 3 end
区间 3 颜色|Range 3 color
强调色|Accent color
角度以正上方为 0°，顺时针为正；起止角度相同表示完整的 360°。|Angles start at 0° at the top and increase clockwise; equal start and end angles represent a full 360° sweep.
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
InfoBar 与 Expander|InfoBar and Expander
InfoBar 用于页面内非阻塞通知；Expander 用于按需显示相关的次要内容。|InfoBar shows non-blocking inline notifications; Expander reveals related secondary content on demand.
信息|Information
新版本已经可以下载。|A new version is ready to download.
查看更新|View update
当前已经是最新版本。|You are already using the latest version.
成功|Success
所有更改均已保存。|All changes have been saved.
警告|Warning
网络连接不稳定，部分内容可能延迟。|The network connection is unstable, so some content may be delayed.
错误|Error
无法连接到服务，请稍后重试。|Unable to connect to the service. Try again later.
重新显示全部通知|Show all notifications again
窗口级弹出|Window-level popups
弹出通知相对主窗口定位；同一位置的新通知贴近窗口边缘，旧通知向中心依次堆叠。|Popup notifications are positioned relative to the main window. New notifications stay at the selected edge while older ones stack toward the center.
窗口级通知 %1|Window notification %1
该通知会在 4.5 秒后关闭；鼠标悬停时暂停计时。|This notification closes after 4.5 seconds; hovering pauses the timer.
左上|Top left
顶部|Top
右上|Top right
左下|Bottom left
底部|Bottom
右下|Bottom right
在右上角连续弹出 10 条|Show 10 at the top right
关闭全部弹出通知|Close all popup notifications
高级设置|Advanced settings
启用自动保存|Enable autosave
启动时恢复上次会话|Restore the previous session at startup
通知音量|Notification volume
同步状态|Sync status
3 个设备|3 devices
桌面电脑、笔记本电脑和移动设备均已在刚刚完成同步。|The desktop, laptop, and mobile device have just finished syncing.
向上展开|Expand upward
内容显示在 Header 上方，适合靠近页面底部的布局。|Content appears above the header, which suits layouts near the bottom of a page.
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
