#pragma once

#include "widgets/waveformwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>

class WaveformPage : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformPage(QWidget *parent = nullptr);

private slots:
    void onDataReceived(const QByteArray &data, const QDateTime &timestamp);
    void onTogglePauseClicked();
    void onClearDataClicked();
    void onSimulateTimer();
    void onToggleSimulateClicked();
    void onPointsChanged(int points);

private:
    void setupUi();
    void parseIncomingText(const QString &text);

    WaveformWidget *m_waveformWidget{nullptr};
    QPushButton *m_pauseBtn{nullptr};
    QPushButton *m_clearBtn{nullptr};
    QPushButton *m_simulateBtn{nullptr};
    QComboBox *m_pointsCombo{nullptr};

    QList<QCheckBox *> m_channelChecks;
    QList<QLineEdit *> m_channelNames;

    QTimer *m_simTimer{nullptr};
    double m_simAngle{0.0};
};
