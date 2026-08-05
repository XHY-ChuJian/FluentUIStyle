#include "installedsoftwaretablewidget.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QDate>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QSettings>
#include <QSet>
#include <QSize>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace {

struct InstalledSoftwareInfo
{
    QString name;
    QString version;
    QString publisher;
    QString installDate;
    QString source;
    QString displayIcon;
    QString uninstallString;
    QString quietUninstallString;
    QString installLocation;
};

QString formatInstallDate(const QString &rawDate)
{
    if (rawDate.size() == 8)
    {
        const QDate date = QDate::fromString(rawDate, QStringLiteral("yyyyMMdd"));
        if (date.isValid())
        {
            return date.toString(QStringLiteral("yyyy-MM-dd"));
        }
    }
    return rawDate;
}

QString normalizeDisplayIconPath(QString value)
{
    value = value.trimmed();
    if (value.isEmpty())
    {
        return {};
    }

    if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
    {
        value = value.mid(1, value.size() - 2);
    }

    const int comma = value.lastIndexOf(QLatin1Char(','));
    if (comma > 0)
    {
        const QString maybeIndex = value.mid(comma + 1).trimmed();
        bool ok = false;
        maybeIndex.toInt(&ok);
        if (ok)
        {
            value = value.left(comma).trimmed();
            if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            {
                value = value.mid(1, value.size() - 2);
            }
        }
    }

    return value;
}

QIcon iconFromDisplayIcon(const QString &displayIcon)
{
    const QString path = normalizeDisplayIconPath(displayIcon);
    if (path.isEmpty())
    {
        return {};
    }

    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile())
    {
        return {};
    }

    QFileIconProvider provider;
    return provider.icon(fi);
}

QString executablePathFromCommandLine(QString value)
{
    value = value.trimmed();
    if (value.isEmpty())
    {
        return {};
    }

    if (value.startsWith(QLatin1Char('"')))
    {
        const int endQuote = value.indexOf(QLatin1Char('"'), 1);
        if (endQuote > 1)
        {
            value = value.mid(1, endQuote - 1);
        }
    }
    else
    {
        const int firstSpace = value.indexOf(QLatin1Char(' '));
        if (firstSpace > 0)
        {
            value = value.left(firstSpace);
        }
    }

    value = value.trimmed();
    if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')) && value.size() >= 2)
    {
        value = value.mid(1, value.size() - 2);
    }

    const QFileInfo fi(value);
    if (!fi.exists() || !fi.isFile())
    {
        return {};
    }
    return fi.absoluteFilePath();
}

QIcon iconFromSoftwareInfo(const InstalledSoftwareInfo &info)
{
    if (const QIcon icon = iconFromDisplayIcon(info.displayIcon); !icon.isNull())
    {
        return icon;
    }

    const QString uninstallExe = executablePathFromCommandLine(info.uninstallString);
    if (!uninstallExe.isEmpty())
    {
        QFileIconProvider provider;
        return provider.icon(QFileInfo(uninstallExe));
    }

    const QString quietExe = executablePathFromCommandLine(info.quietUninstallString);
    if (!quietExe.isEmpty())
    {
        QFileIconProvider provider;
        return provider.icon(QFileInfo(quietExe));
    }

    const QString loc = info.installLocation.trimmed();
    if (!loc.isEmpty())
    {
        const QDir dir(loc);
        if (dir.exists())
        {
            const QFileInfoList exeList = dir.entryInfoList(QStringList() << QStringLiteral("*.exe"),
                                                            QDir::Files | QDir::NoDotAndDotDot);
            if (!exeList.isEmpty())
            {
                QFileIconProvider provider;
                return provider.icon(exeList.first());
            }
        }
    }

    return {};
}

void appendInstalledSoftwareFromRegistry(const QString &rootPath,
                                       const QString &sourceLabel,
                                       QList<InstalledSoftwareInfo> &items,
                                       QSet<QString> &dedupeKeys)
{
    QSettings reg(rootPath, QSettings::NativeFormat);
    const QStringList subKeys = reg.childGroups();
    for (const QString &subKey : subKeys)
    {
        reg.beginGroup(subKey);

        const QString displayName = reg.value(QStringLiteral("DisplayName")).toString().trimmed();
        const bool systemComponent = reg.value(QStringLiteral("SystemComponent"), QVariant(0)).toInt() == 1;
        const QString releaseType = reg.value(QStringLiteral("ReleaseType")).toString();
        const QString parentKeyName = reg.value(QStringLiteral("ParentKeyName")).toString();

        if (displayName.isEmpty() || systemComponent || !parentKeyName.isEmpty()
            || releaseType.contains(QStringLiteral("Update"), Qt::CaseInsensitive)
            || releaseType.contains(QStringLiteral("Hotfix"), Qt::CaseInsensitive))
        {
            reg.endGroup();
            continue;
        }

        const QString version = reg.value(QStringLiteral("DisplayVersion")).toString().trimmed();
        const QString publisher = reg.value(QStringLiteral("Publisher")).toString().trimmed();
        const QString installDate = formatInstallDate(reg.value(QStringLiteral("InstallDate")).toString().trimmed());
        const QString displayIcon = reg.value(QStringLiteral("DisplayIcon")).toString().trimmed();
        const QString uninstallString = reg.value(QStringLiteral("UninstallString")).toString().trimmed();
        const QString quietUninstallString = reg.value(QStringLiteral("QuietUninstallString")).toString().trimmed();
        const QString installLocation = reg.value(QStringLiteral("InstallLocation")).toString().trimmed();

        const QString dedupeKey = displayName + QLatin1Char('|') + version + QLatin1Char('|') + publisher;
        if (dedupeKeys.contains(dedupeKey))
        {
            reg.endGroup();
            continue;
        }
        dedupeKeys.insert(dedupeKey);

        InstalledSoftwareInfo item;
        item.name = displayName;
        item.version = version;
        item.publisher = publisher;
        item.installDate = installDate;
        item.source = sourceLabel;
        item.displayIcon = displayIcon;
        item.uninstallString = uninstallString;
        item.quietUninstallString = quietUninstallString;
        item.installLocation = installLocation;
        items.append(item);

        reg.endGroup();
    }
}

QList<InstalledSoftwareInfo> queryInstalledSoftwareList()
{
    QList<InstalledSoftwareInfo> items;
    QSet<QString> dedupeKeys;

    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKLM 64-bit"), items, dedupeKeys);
    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKLM 32-bit"), items, dedupeKeys);
    appendInstalledSoftwareFromRegistry(
        QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKCU"), items, dedupeKeys);

    std::sort(items.begin(), items.end(), [](const InstalledSoftwareInfo &a, const InstalledSoftwareInfo &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    return items;
}

} // namespace

InstalledSoftwareTableWidget::InstalledSoftwareTableWidget(QTableWidget *table, QObject *parent)
    : QObject(parent)
    , m_table(table)
{
}

void InstalledSoftwareTableWidget::initialize()
{
    if (!m_table)
    {
        return;
    }

    QTableWidget *table = m_table;
    table->clear();
    constexpr int kTableColumnCount = 5;
    table->setColumnCount(kTableColumnCount);

    QStringList headers;
    headers << tr("软件名称") << tr("版本") << tr("发布商") << tr("安装日期") << tr("来源");
    table->setHorizontalHeaderLabels(headers);

    table->verticalHeader()->setMinimumSectionSize(50);
    table->verticalHeader()->setDefaultSectionSize(50);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setFixedHeight(50);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setIconSize(QSize(20, 20));

    {
        QFont headerFont = table->horizontalHeader()->font();
        headerFont.setPixelSize(14);
        table->horizontalHeader()->setFont(headerFont);
    }

    const QList<InstalledSoftwareInfo> softwareList = queryInstalledSoftwareList();
    table->setRowCount(softwareList.size());

    for (int row = 0; row < softwareList.size(); ++row)
    {
        const InstalledSoftwareInfo &app = softwareList.at(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(app.name);
        const QIcon appIcon = iconFromSoftwareInfo(app);
        if (!appIcon.isNull())
        {
            nameItem->setIcon(appIcon);
        }
        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, new QTableWidgetItem(app.version.isEmpty() ? QStringLiteral("-") : app.version));
        table->setItem(row, 2, new QTableWidgetItem(app.publisher.isEmpty() ? QStringLiteral("-") : app.publisher));
        table->setItem(row, 3, new QTableWidgetItem(app.installDate.isEmpty() ? QStringLiteral("-") : app.installDate));
        table->setItem(row, 4, new QTableWidgetItem(app.source));
    }

    table->resizeColumnsToContents();
    if (!softwareList.isEmpty())
    {
        table->selectRow(0);
    }
    else
    {
        table->setRowCount(1);
        table->setItem(0, 0, new QTableWidgetItem(tr("未读取到安装软件信息")));
        for (int col = 1; col < table->columnCount(); ++col)
        {
            table->setItem(0, col, new QTableWidgetItem(QStringLiteral("-")));
        }
    }
}
