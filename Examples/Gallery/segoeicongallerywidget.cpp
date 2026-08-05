#include "segoeicongallerywidget.h"

#include "font-icon/fonticon.h"

#include <QFile>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

SegoeIconGalleryWidget::SegoeIconGalleryWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    initializeUi();
    populateTable();
}

void SegoeIconGalleryWidget::initializeUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(9, 9, 9, 9);
    layout->setSpacing(14);

    QLabel *titleLabel = new QLabel(QStringLiteral("Segoe Fluent Icons 图标"), this);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 18pt; font-weight: 800;"));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("输入名称筛选"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(220);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setBackgroundRole(QPalette::Base);
    m_tableWidget->setAutoFillBackground(true);
    m_tableWidget->setColumnCount(10);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_tableWidget->setShowGrid(false);
    m_tableWidget->setFocusPolicy(Qt::NoFocus);
    m_tableWidget->setFrameShape(QFrame::NoFrame);
    m_tableWidget->setWordWrap(true);
    m_tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->horizontalHeader()->setVisible(false);
    m_tableWidget->horizontalHeader()->setDefaultSectionSize(110);
    m_tableWidget->horizontalHeader()->setMinimumSectionSize(78);
    QWidget *corner = new QWidget(m_tableWidget);
    corner->setObjectName(QStringLiteral("tableCornerSpacer"));
    corner->setAttribute(Qt::WA_StyledBackground, true);
    corner->setStyleSheet(QStringLiteral("#tableCornerSpacer { background: transparent; border: none; }"));
    m_tableWidget->setCornerWidget(corner);

    QHBoxLayout *topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->addWidget(titleLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_searchEdit);

    connect(m_searchEdit,
            &QLineEdit::textChanged,
            this,
            [this](const QString &text)
            {
                populateTable(text);
            });

    layout->addLayout(topBarLayout);
    layout->addWidget(m_tableWidget, 1);
}

void SegoeIconGalleryWidget::populateTable(const QString &keyword)
{
    const QList<IconEntry> entries = iconEntries();
    const QString filter = keyword.trimmed().toLower();

    QList<int> matchedIndexes;
    matchedIndexes.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i)
    {
        const QString name = entries[i].name.toLower();
        if (filter.isEmpty() || name.contains(filter))
        {
            matchedIndexes.append(i);
        }
    }

    const int columnCount = m_tableWidget->columnCount();
    const int rowCount = (matchedIndexes.size() + columnCount - 1) / columnCount;
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(rowCount);

    QColor iconColor = palette().color(QPalette::WindowText);
    const bool darkLike = palette().color(QPalette::Base).lightness() < 128;
    iconColor.setAlpha(darkLike ? 210 : 180);

    for (int row = 0; row < rowCount; ++row)
    {
        m_tableWidget->setRowHeight(row, 86);
    }

    for (int i = 0; i < matchedIndexes.size(); ++i)
    {
        const IconEntry &entry = entries[matchedIndexes.at(i)];
        const int row = i / columnCount;
        const int col = i % columnCount;

        QWidget *cellWidget = new QWidget(m_tableWidget);
        QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(4, 4, 4, 4);
        cellLayout->setSpacing(5);

        QLabel *iconLabel = new QLabel(cellWidget);
        iconLabel->setAlignment(Qt::AlignCenter);
        QFont iconFont(QString::fromLatin1(SegoeFontName));
        iconFont.setPixelSize(30);
        iconFont.setHintingPreference(QFont::PreferNoHinting);
        iconLabel->setFont(iconFont);
        iconLabel->setText(QString(QChar(static_cast<char16_t>(entry.code))));
        iconLabel->setMinimumHeight(36);

        QLabel *nameLabel = new QLabel(entry.name, cellWidget);
        nameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        nameLabel->setWordWrap(true);

        cellLayout->addWidget(iconLabel);
        cellLayout->addWidget(nameLabel);
        m_tableWidget->setCellWidget(row, col, cellWidget);
    }
}

QList<SegoeIconGalleryWidget::IconEntry> SegoeIconGalleryWidget::iconEntries()
{
    static QList<IconEntry> entries;
    if (!entries.isEmpty())
    {
        return entries;
    }

    QFile enumFile(QStringLiteral(":/resource/SegoeFluentIconsEnum.txt"));
    if (!enumFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return entries;
    }

    const QRegularExpression enumLinePattern(
        QStringLiteral("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\s*=\\s*0x([0-9A-Fa-f]+)\\s*,?\\s*$"));

    QTextStream stream(&enumFile);
    while (!stream.atEnd())
    {
        const QString line = stream.readLine();
        const QRegularExpressionMatch match = enumLinePattern.match(line);
        if (!match.hasMatch())
        {
            continue;
        }

        bool ok = false;
        const int code = match.captured(2).toInt(&ok, 16);
        if (!ok)
        {
            continue;
        }

        entries.append({match.captured(1), code});
    }

    return entries;
}

