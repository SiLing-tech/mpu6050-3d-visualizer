#include "SensorDataParser.h"
#include <QStringList>

SensorData SensorDataParser::parse(const QString &line)
{
    SensorData data;
    const QStringList tokens = line.split(',');

    for (const QString &token : tokens) {
        const int colon = token.indexOf(':');
        if (colon < 0)
            continue;

        const QString key = token.left(colon).trimmed();
        const float value = token.mid(colon + 1).trimmed().toFloat();

        if (key == "ACC_X")
            data.accelX = value;
        else if (key == "ACC_Y")
            data.accelY = value;
        else if (key == "ACC_Z")
            data.accelZ = value;
        else if (key == "GYRO_X")
            data.gyroX = value;
        else if (key == "GYRO_Y")
            data.gyroY = value;
        else if (key == "GYRO_Z")
            data.gyroZ = value;
        else if (key == "TEMP")
            data.temp = value;
    }

    data.valid = true;
    return data;
}
