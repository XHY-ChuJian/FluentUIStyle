#pragma once

#include "core/serialengine.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

class SerialControlBar : public QFrame
{
    Q_OBJECT

public:
    explicit SerialControlBar(QWidget *parent = nullptr);

    void refreshPorts();

private slots:
    void onTogglePortClicked();
    void onResetCountersClicked();
    void onPortsChanged(const QList<QSerialPortInfo> &ports);
    void onPortOpened(const QString &portName, qint32 baud);
    void onPortClosed();
    void onCountersUpdated(qint64 rx, qint64 tx, qint64 rxFrames, qint64 txFrames);

private:
    void setupUi();
    void updateControlsEnabled(bool opened);

    QComboBox *m_portCombo{nullptr};
    QComboBox *m_baudCombo{nullptr};
    QComboBox *m_dataBitsCombo{nullptr};
    QComboBox *m_parityCombo{nullptr};
    QComboBox *m_stopBitsCombo{nullptr};
    QComboBox *m_flowCombo{nullptr};

    QPushButton *m_openBtn{nullptr};
    QPushButton *m_refreshBtn{nullptr};
    QPushButton *m_resetCountersBtn{nullptr};

    QLabel *m_statusDot{nullptr};
    QLabel *m_statusText{nullptr};
    QLabel *m_rxCountLabel{nullptr};
    QLabel *m_txCountLabel{nullptr};
};
