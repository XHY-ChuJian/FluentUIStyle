#pragma once

#include <QObject>

class QTableWidget;

class InstalledSoftwareTableWidget : public QObject
{
    Q_OBJECT

public:
    explicit InstalledSoftwareTableWidget(QTableWidget *table, QObject *parent = nullptr);

    void initialize();

private:
    QTableWidget *m_table{nullptr};
};
