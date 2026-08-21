#include "crcutils.h"

#include <QDataStream>
#include <QRegularExpression>
#include <QtEndian>
#include <cstring>

namespace CrcUtils {

quint16 crc16Modbus(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    int len = data.size();

    for (int i = 0; i < len; ++i) {
        crc ^= ptr[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

quint16 crc16Ccitt(const QByteArray &data, quint16 init)
{
    quint16 crc = init;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    int len = data.size();

    for (int i = 0; i < len; ++i) {
        crc ^= (quint16)ptr[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

quint16 crc16Xmodem(const QByteArray &data)
{
    return crc16Ccitt(data, 0x0000);
}

quint32 crc32(const QByteArray &data)
{
    quint32 crc = 0xFFFFFFFF;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    int len = data.size();

    for (int i = 0; i < len; ++i) {
        crc ^= ptr[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

quint8 checksum8(const QByteArray &data)
{
    quint32 sum = 0;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    for (int i = 0; i < data.size(); ++i) {
        sum += ptr[i];
    }
    return static_cast<quint8>(sum & 0xFF);
}

quint16 checksum16(const QByteArray &data)
{
    quint32 sum = 0;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    for (int i = 0; i < data.size(); ++i) {
        sum += ptr[i];
    }
    return static_cast<quint16>(sum & 0xFFFF);
}

quint8 xor8(const QByteArray &data)
{
    quint8 x = 0;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    for (int i = 0; i < data.size(); ++i) {
        x ^= ptr[i];
    }
    return x;
}

quint8 lrc(const QByteArray &data)
{
    quint8 sum = 0;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.constData());
    for (int i = 0; i < data.size(); ++i) {
        sum += ptr[i];
    }
    return static_cast<quint8>((0x100 - sum) & 0xFF);
}

QByteArray hexStringToByteArray(const QString &hexStr)
{
    QString cleaned = hexStr;
    cleaned.remove(QRegularExpression(QStringLiteral("[^0-9a-fA-F]")));
    if (cleaned.length() % 2 != 0) {
        cleaned.prepend(QLatin1Char('0'));
    }
    return QByteArray::fromHex(cleaned.toLatin1());
}

QString byteArrayToHexString(const QByteArray &data, bool upper, const QString &sep)
{
    if (data.isEmpty()) {
        return QString();
    }
    QString hex = data.toHex();
    if (upper) {
        hex = hex.toUpper();
    }

    if (sep.isEmpty()) {
        return hex;
    }

    QString result;
    result.reserve(hex.size() + (hex.size() / 2) * sep.size());
    for (int i = 0; i < hex.size(); i += 2) {
        if (i > 0) {
            result += sep;
        }
        result += hex.mid(i, 2);
    }
    return result;
}

QString floatToHex(float val, bool bigEndian)
{
    union {
        float f;
        quint32 u;
    } conv;
    conv.f = val;

    quint32 raw = conv.u;
    if (!bigEndian) {
        raw = qbswap(raw);
    }
    return QStringLiteral("%1").arg(raw, 8, 16, QLatin1Char('0')).toUpper();
}

float hexToFloat(const QString &hexStr, bool bigEndian, bool *ok)
{
    QString cleaned = hexStr;
    cleaned.remove(QRegularExpression(QStringLiteral("[^0-9a-fA-F]")));
    if (cleaned.length() > 8) {
        cleaned = cleaned.left(8);
    }
    while (cleaned.length() < 8) {
        cleaned.prepend(QLatin1Char('0'));
    }

    bool convOk = false;
    quint32 raw = cleaned.toUInt(&convOk, 16);
    if (!convOk) {
        if (ok) *ok = false;
        return 0.0f;
    }

    if (!bigEndian) {
        raw = qbswap(raw);
    }

    union {
        quint32 u;
        float f;
    } conv;
    conv.u = raw;

    if (ok) *ok = true;
    return conv.f;
}

QByteArray buildModbusRtuRequest(quint8 slaveAddr, quint8 funcCode, quint16 regAddr, quint16 countOrVal, const QByteArray &extraData)
{
    QByteArray frame;
    frame.append(static_cast<char>(slaveAddr));
    frame.append(static_cast<char>(funcCode));
    frame.append(static_cast<char>((regAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(regAddr & 0xFF));
    frame.append(static_cast<char>((countOrVal >> 8) & 0xFF));
    frame.append(static_cast<char>(countOrVal & 0xFF));

    if (!extraData.isEmpty()) {
        frame.append(extraData);
    }

    quint16 crc = crc16Modbus(frame);
    // Modbus RTU CRC 低字节在前，高字节在后
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    return frame;
}

} // namespace CrcUtils
