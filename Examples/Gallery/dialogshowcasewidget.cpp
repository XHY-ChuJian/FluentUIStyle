#include "dialogshowcasewidget.h"
#include "qdebug.h"

#include <QCoreApplication>
#include <QList>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <exmessagebox.h>
#include <excontentdialog.h>
#include <QCheckBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QThread>
#include <QVBoxLayout>

namespace
{
    QWidget *dialogParent(QWidget *w)
    {
        QWidget *top = w ? w->window() : nullptr;
        return top ? top : w;
    }

    /** Fluent-style card block: QWidget isCard + section title + horizontal button row. */
    QHBoxLayout *addCardSection(QVBoxLayout *mainLay, QWidget *contentParent, const QString &sectionTitle)
    {
        auto *card = new QWidget(contentParent);
        card->setProperty("isCard", true);
        card->setAttribute(Qt::WA_StyledBackground, true);

        auto *v = new QVBoxLayout(card);
        v->setContentsMargins(12, 12, 12, 12);
        v->setSpacing(10);

        auto *title = new QLabel(sectionTitle, card);
        {
            QFont f = title->font();
            f.setBold(true);
            f.setPixelSize(14);
            title->setFont(f);
        }
        v->addWidget(title);

        auto *row = new QHBoxLayout;
        row->setSpacing(8);
        row->addStretch(0);
        v->addLayout(row);

        mainLay->addWidget(card);
        return row;
    }
} // namespace

DialogShowcaseWidget::DialogShowcaseWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setAutoFillBackground(false);

    auto *content = new QWidget(scroll);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);

    auto *mainLay = new QVBoxLayout(content);
    mainLay->setContentsMargins(16, 16, 16, 16);
    mainLay->setSpacing(16);

    auto *title = new QLabel(tr("常用对话框（Qt Widgets）"), content);
    {
        QFont f = title->font();
        f.setPointSize(16);
        f.setBold(true);
        title->setFont(f);
    }
    mainLay->addWidget(title);

    auto *hint = new QLabel(tr("以下按钮会弹出模态或非模态对话框，用于在 Fluent 样式下查看常见 Qt 对话框外观。"), content);
    hint->setWordWrap(true);
    mainLay->addWidget(hint);

    auto addButtons = [](QHBoxLayout *row, const QList<QPushButton *> &buttons)
    {
        for (QPushButton *b : buttons)
        {
            row->insertWidget(row->count() - 1, b);
        }
    };

    // --- QMessageBox ---
    QHBoxLayout *rowMsg = addCardSection(mainLay, content, tr("消息框 (QMessageBox)"));
    auto *btnInfo = new QPushButton(tr("信息"), content);
    auto *btnWarn = new QPushButton(tr("警告"), content);
    auto *btnCrit = new QPushButton(tr("严重"), content);
    auto *btnQuest = new QPushButton(tr("询问"), content);
    auto *btnSavePrompt = new QPushButton(tr("是否保存文件…"), content);
    auto *btnDetailInfo = new QPushButton(tr("详细信息"), content);
    auto *btnCheckBox = new QPushButton(tr("复选框"), content);
    auto *btnPassword = new QPushButton(tr("密码输入"), content);

    connect(btnInfo, &QPushButton::clicked, this, [this]()
            { ExMessageBox::information(dialogParent(this), tr("信息"), tr("这是一条信息消息。")); });
    connect(btnWarn, &QPushButton::clicked, this, [this]()
            { ExMessageBox::warning(dialogParent(this), tr("警告"), tr("这是一条警告消息。")); });
    connect(btnCrit, &QPushButton::clicked, this, [this]()
            { ExMessageBox::critical(dialogParent(this), tr("严重"), tr("这是一条严重错误消息。")); });
    connect(btnQuest, &QPushButton::clicked, this, [this]()
            { ExMessageBox::question(dialogParent(this), tr("询问"), tr("是否继续？")); });
    connect(btnSavePrompt, &QPushButton::clicked, this, [this]()
            { ExMessageBox::question(dialogParent(this),
                                     tr("保存文件"),
                                     tr("文档已修改，是否在关闭前保存？"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Save); });
    connect(btnDetailInfo, &QPushButton::clicked, this, [this]()
            {
                ExMessageBox msgBox(ExMessageBox::Information, tr("操作结果"), tr("文件已成功导出。"), QMessageBox::Ok, dialogParent(this));
                msgBox.setInformativeText(tr("该文件保存在以下路径:\nC:\\Users\\Public\\Documents\\Report.pdf"));
                msgBox.exec();
            });
    connect(btnCheckBox, &QPushButton::clicked, this, [this]()
            {
                ExMessageBox msgBox(ExMessageBox::Warning, tr("安全警告"), tr("您即将执行一个危险操作。"), QMessageBox::Yes | QMessageBox::No, dialogParent(this));
                msgBox.setInformativeText(tr("继续执行可能会导致数据丢失。"));
                QCheckBox cb(tr("不再提醒我"));
                msgBox.setCheckBox(&cb);
                qDebug() << msgBox.exec();
            });

    connect(btnPassword, &QPushButton::clicked, this, [this]()
            {
                ExMessageBox msgBox(ExMessageBox::Information, tr("验证身份"), tr("请输入管理员密码以继续操作。"), QMessageBox::Ok | QMessageBox::Cancel, dialogParent(this));
                auto *lineEdit = new QLineEdit(&msgBox);
                lineEdit->setEchoMode(QLineEdit::Password);
                lineEdit->setPlaceholderText(tr("输入密码"));
                msgBox.setContentWidget(lineEdit);
                qDebug() << msgBox.exec();
            });

    addButtons(rowMsg, {btnInfo, btnWarn, btnCrit, btnQuest, btnSavePrompt, btnDetailInfo, btnCheckBox, btnPassword});

    // --- QInputDialog ---
    QHBoxLayout *rowIn = addCardSection(mainLay, content, tr("输入框 (QInputDialog)"));
    auto *btnText = new QPushButton(tr("单行文本"), content);
    auto *btnInt = new QPushButton(tr("整数"), content);
    auto *btnDouble = new QPushButton(tr("浮点数"), content);
    auto *btnItem = new QPushButton(tr("列表选择"), content);
    auto *btnMulti = new QPushButton(tr("多行文本"), content);
    connect(btnText, &QPushButton::clicked, this, [this]()
            { QInputDialog::getText(dialogParent(this), tr("输入文本"), tr("请输入内容："), QLineEdit::Normal, tr("示例")); });
    connect(btnInt, &QPushButton::clicked, this, [this]()
            { QInputDialog::getInt(dialogParent(this), tr("输入整数"), tr("数值："), 42, -1000, 1000, 1); });
    connect(btnDouble, &QPushButton::clicked, this, [this]()
            { QInputDialog::getDouble(dialogParent(this), tr("输入浮点数"), tr("数值："), 3.14, -1000.0, 1000.0, 2); });
    connect(btnItem, &QPushButton::clicked, this, [this]()
            {
                const QStringList items{tr("选项 A"), tr("选项 B"), tr("选项 C")};
                QInputDialog::getItem(dialogParent(this), tr("选择一项"), tr("请选择："), items, 0, false);
            });
    connect(btnMulti, &QPushButton::clicked, this, [this]()
            { QInputDialog::getMultiLineText(dialogParent(this), tr("多行输入"), tr("内容："), tr("第一行\n第二行")); });
    addButtons(rowIn, {btnText, btnInt, btnDouble, btnItem, btnMulti});

    // --- QColorDialog / QFontDialog ---
    QHBoxLayout *rowPick = addCardSection(mainLay, content, tr("颜色与字体"));
    auto *btnColor = new QPushButton(tr("选择颜色…"), content);
    auto *btnFont = new QPushButton(tr("选择字体…"), content);
    connect(btnColor, &QPushButton::clicked, this, [this]()
            { QColorDialog::getColor(Qt::blue, dialogParent(this), tr("选择颜色")); });
    connect(btnFont, &QPushButton::clicked, this, [this]()
            {
                bool ok = false;
                QFontDialog::getFont(&ok, dialogParent(this)->font(), dialogParent(this), tr("选择字体"));
            });
    addButtons(rowPick, {btnColor, btnFont});

    // --- QProgressDialog ---
    QHBoxLayout *rowProg = addCardSection(mainLay, content, tr("进度对话框 (QProgressDialog)"));
    auto *btnProg = new QPushButton(tr("短时进度…"), content);
    connect(btnProg, &QPushButton::clicked, this, [this]()
            {
                QProgressDialog dlg(tr("正在处理…"), tr("取消"), 0, 100, dialogParent(this));
                dlg.setWindowModality(Qt::WindowModal);
                dlg.setMinimumDuration(0);
                dlg.setValue(0);
                for (int i = 0; i <= 100; ++i)
                {
                    dlg.setValue(i);
                    QCoreApplication::processEvents();
                    if (dlg.wasCanceled())
                    {
                        break;
                    }
                    QThread::msleep(8);
                }
            });
    addButtons(rowProg, {btnProg});

    // --- QDialog + QDialogButtonBox ---
    QHBoxLayout *rowCustom = addCardSection(mainLay, content, tr("自定义对话框 (QDialog + QDialogButtonBox)"));
    auto *btnCustom = new QPushButton(tr("打开示例对话框…"), content);
    connect(btnCustom, &QPushButton::clicked, this, [this]()
            {
                QDialog dlg(dialogParent(this));
                dlg.setWindowTitle(tr("示例对话框"));
                auto *v = new QVBoxLayout(&dlg);
                v->addWidget(new QLabel(tr("这是一个带 QDialogButtonBox 的简单对话框。"), &dlg));
                auto *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                QObject::connect(bbox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                QObject::connect(bbox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
                v->addWidget(bbox);
                dlg.resize(420, 140);
                dlg.exec();
            });
    addButtons(rowCustom, {btnCustom});

    // --- ExContentDialog (WinUI 3 ContentDialog) ---
    QHBoxLayout *rowCD = addCardSection(mainLay, content, tr("ContentDialog (WinUI 3 风格)"));

    auto *btnCDSimple = new QPushButton(tr("简单通知"), content);
    auto *btnCDConfirm = new QPushButton(tr("确认删除"), content);
    auto *btnCDThree = new QPushButton(tr("三按钮订阅"), content);
    auto *btnCDCustom = new QPushButton(tr("自定义内容"), content);

    // 1. 简单通知 — 只有 CloseButton
    connect(btnCDSimple, &QPushButton::clicked, this, [this]()
            {
                ExContentDialog dlg(dialogParent(this));
                dlg.setTitle(tr("没有 Wi-Fi 连接"));
                dlg.setContent(tr("请检查网络连接后重试。"));
                dlg.setCloseButtonText(tr("确定"));
                dlg.setDefaultButton(ExContentDialog::CloseBtn);
                dlg.showDialog();
            });

    // 2. 确认删除 — PrimaryButton + CloseButton
    connect(btnCDConfirm, &QPushButton::clicked, this, [this]()
            {
                ExContentDialog dlg(dialogParent(this));
                dlg.setTitle(tr("永久删除文件？"));
                dlg.setContent(tr("如果删除此文件，将无法恢复。\n确定要删除吗？"));
                dlg.setPrimaryButtonText(tr("删除"));
                dlg.setCloseButtonText(tr("取消"));
                dlg.setDefaultButton(ExContentDialog::CloseBtn);

                auto result = dlg.showDialog();
                if (result == ExContentDialog::Primary)
                {
                    // 用户选择了删除
                }
            });

    // 3. 三按钮订阅 — Primary + Secondary + Close
    connect(btnCDThree, &QPushButton::clicked, this, [this]()
            {
                ExContentDialog dlg(dialogParent(this));
                dlg.setTitle(tr("订阅应用服务？"));
                dlg.setContent(tr("以每月 ¥9.99 的价格享受高清视听体验。\n免费试用，随时取消。"));
                dlg.setPrimaryButtonText(tr("订阅"));
                dlg.setSecondaryButtonText(tr("免费试用"));
                dlg.setCloseButtonText(tr("暂时不要"));
                dlg.setDefaultButton(ExContentDialog::PrimaryBtn);
                dlg.showDialog();
            });

    // 4. 自定义内容 — 在 Content 区域放置自定义 Widget
    connect(btnCDCustom, &QPushButton::clicked, this, [this]()
            {
                ExContentDialog dlg(dialogParent(this));
                dlg.setTitle(tr("保存更改"));

                auto *customWidget = new QWidget;
                auto *lay = new QVBoxLayout(customWidget);
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(8);
                lay->addWidget(new QLabel(tr("以下文件尚未保存：")));

                auto *fileList = new QLabel(
                    QStringLiteral("  • document.txt\n  • image.png\n  • settings.json"));
                QFont f = fileList->font();
                f.setPixelSize(13);
                fileList->setFont(f);
                lay->addWidget(fileList);
                lay->addWidget(new QLabel(tr("是否在关闭前保存这些文件？")));

                dlg.setContentWidget(customWidget);
                dlg.setPrimaryButtonText(tr("全部保存"));
                dlg.setSecondaryButtonText(tr("不保存"));
                dlg.setCloseButtonText(tr("取消"));
                dlg.setDefaultButton(ExContentDialog::PrimaryBtn);
                dlg.showDialog();
            });

    addButtons(rowCD, {btnCDSimple, btnCDConfirm, btnCDThree, btnCDCustom});

    mainLay->addStretch(1);

    scroll->setWidget(content);
    content->setAutoFillBackground(false);
}
