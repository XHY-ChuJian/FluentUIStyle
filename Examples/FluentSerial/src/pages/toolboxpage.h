#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QWidget>

class ToolboxPage : public QWidget
{
    Q_OBJECT

public:
    explicit ToolboxPage(QWidget *parent = nullptr);

private slots:
    void onCrcInputChanged();
    void onFloatToHexClicked();
    void onHexToFloatClicked();
    void onBuildModbusFrameClicked();
    void onSendModbusToSerial();

private:
    void setupUi();

    // 1. CRC 计算器
    QPlainTextEdit *m_crcInputEdit{nullptr};
    QRadioButton *m_crcInputHexRadio{nullptr};
    QRadioButton *m_crcInputTextRadio{nullptr};

    QLineEdit *m_crc16ModbusLeEdit{nullptr};
    QLineEdit *m_crc16ModbusBeEdit{nullptr};
    QLineEdit *m_crc16CcittEdit{nullptr};
    QLineEdit *m_crc16XmodemEdit{nullptr};
    QLineEdit *m_crc32Edit{nullptr};
    QLineEdit *m_sum8Edit{nullptr};
    QLineEdit *m_xor8Edit{nullptr};
    QLineEdit *m_lrcEdit{nullptr};

    // 2. IEEE 754 浮点转换
    QLineEdit *m_floatInputEdit{nullptr};
    QLineEdit *m_hexOutputBeEdit{nullptr};
    QLineEdit *m_hexOutputLeEdit{nullptr};
    QPushButton *m_floatToHexBtn{nullptr};

    QLineEdit *m_hexInputEdit{nullptr};
    QLineEdit *m_floatOutputEdit{nullptr};
    QPushButton *m_hexToFloatBtn{nullptr};

    // 3. Modbus RTU 组帧器
    QSpinBox *m_mbSlaveSpin{nullptr};
    QComboBox *m_mbFuncCombo{nullptr};
    QSpinBox *m_mbRegAddrSpin{nullptr};
    QSpinBox *m_mbCountSpin{nullptr};
    QLineEdit *m_mbResultEdit{nullptr};
    QPushButton *m_mbBuildBtn{nullptr};
    QPushButton *m_mbSendBtn{nullptr};
};
