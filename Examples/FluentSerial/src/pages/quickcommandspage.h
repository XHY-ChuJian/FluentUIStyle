#pragma once

#include "core/serialengine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

class QuickCommandsPage : public QWidget
{
    Q_OBJECT

public:
    explicit QuickCommandsPage(QWidget *parent = nullptr);

private slots:
    void onAddRowClicked();
    void onClearAllClicked();
    void onExportJsonClicked();
    void onImportJsonClicked();
    void onStartSingleRunClicked();
    void onStartLoopRunClicked();
    void onStopRunClicked();
    void onSendSingleRow(int row);
    void onSequenceProgress(int current, int total, const QString &cmdName);
    void onSequenceFinished();

private:
    void setupUi();
    void loadDefaultPresets();
    void addCommandRow(const SerialCommandItem &item);
    QList<SerialCommandItem> collectCommandsFromTable() const;

    QTableWidget *m_table{nullptr};
    QPushButton *m_addRowBtn{nullptr};
    QPushButton *m_clearAllBtn{nullptr};
    QPushButton *m_importBtn{nullptr};
    QPushButton *m_exportBtn{nullptr};

    QPushButton *m_singleRunBtn{nullptr};
    QPushButton *m_loopRunBtn{nullptr};
    QPushButton *m_stopRunBtn{nullptr};

    QProgressBar *m_progressBar{nullptr};
    QLabel *m_statusLabel{nullptr};
};
