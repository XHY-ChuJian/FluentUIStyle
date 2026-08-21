#pragma once

#include "systemprovider.h"

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

class ProcessesPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessesPage(QWidget *parent = nullptr);

public slots:
    void refreshProcessList();

private slots:
    void onFilterTextChanged(const QString &text);
    void onKillProcessClicked();

private:
    void setupUi();
    void updateTableData();

    QLineEdit *m_searchEdit = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QPushButton *m_killBtn = nullptr;
    QLabel *m_countLabel = nullptr;
    QTableWidget *m_tableWidget = nullptr;

    QList<ProcessItem> m_cachedProcesses;
};
