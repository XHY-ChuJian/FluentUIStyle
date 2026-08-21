#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>

class MonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorPage(QWidget *parent = nullptr);

private slots:
    void onDataReceived(const QByteArray &data, const QDateTime &timestamp);
    void onDataSent(const QByteArray &data, const QDateTime &timestamp);
    void onSendClicked();
    void onClearRxClicked();
    void onExportLogClicked();
    void onAutoSendToggled(bool checked);
    void onAppendCrcClicked();

private:
    void setupUi();
    void appendLog(const QByteArray &data, bool isRx, const QDateTime &time);

    // 接收组件
    QPlainTextEdit *m_rxTextEdit{nullptr};
    QCheckBox *m_rxHexCheck{nullptr};
    QCheckBox *m_timestampCheck{nullptr};
    QCheckBox *m_autoScrollCheck{nullptr};
    QPushButton *m_clearRxBtn{nullptr};
    QPushButton *m_exportLogBtn{nullptr};

    // 发送组件
    QPlainTextEdit *m_txTextEdit{nullptr};
    QCheckBox *m_txHexCheck{nullptr};
    QComboBox *m_suffixCombo{nullptr};
    QCheckBox *m_autoSendCheck{nullptr};
    QSpinBox *m_intervalSpin{nullptr};
    QComboBox *m_crcTypeCombo{nullptr};
    QPushButton *m_appendCrcBtn{nullptr};
    QPushButton *m_clearTxBtn{nullptr};
    QPushButton *m_sendBtn{nullptr};

    QTimer *m_autoSendTimer{nullptr};
};
