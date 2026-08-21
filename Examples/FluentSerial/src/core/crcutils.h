#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace CrcUtils {

// CRC-16 Modbus (Poly: 0x8005, Init: 0xFFFF, RefIn: true, RefOut: true, XorOut: 0x0000)
quint16 crc16Modbus(const QByteArray &data);

// CRC-16 CCITT (Poly: 0x1021, Init: 0x0000 / 0xFFFF)
quint16 crc16Ccitt(const QByteArray &data, quint16 init = 0x0000);

// CRC-16 XMODEM (Poly: 0x1021, Init: 0x0000)
quint16 crc16Xmodem(const QByteArray &data);

// CRC-32 (IEEE 802.3)
quint32 crc32(const QByteArray &data);

// 累加和 8 位 (Sum8)
quint8 checksum8(const QByteArray &data);

// 累加和 16 位 (Sum16)
quint16 checksum16(const QByteArray &data);

// 异或校验 (XOR / BCC)
quint8 xor8(const QByteArray &data);

// 纵向冗余校验 (LRC，Modbus ASCII 常用)
quint8 lrc(const QByteArray &data);

// 字符串/十六进制辅助转换
QByteArray hexStringToByteArray(const QString &hexStr);
QString byteArrayToHexString(const QByteArray &data, bool upper = true, const QString &sep = QStringLiteral(" "));

// IEEE 754 单精度浮点数转换 (float <-> Hex)
QString floatToHex(float val, bool bigEndian = true);
float hexToFloat(const QString &hexStr, bool bigEndian = true, bool *ok = nullptr);

// Modbus RTU 组帧器
// 功能码：01/02读线圈/离散输入, 03/04读保持/输入寄存器, 05写单线圈, 06写单寄存器, 10(16)写多寄存器
QByteArray buildModbusRtuRequest(quint8 slaveAddr, quint8 funcCode, quint16 regAddr, quint16 countOrVal, const QByteArray &extraData = QByteArray());

} // namespace CrcUtils
