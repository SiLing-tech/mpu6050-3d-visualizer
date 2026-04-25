#pragma once

#include <QString>
#include "SensorData.h"

class SensorDataParser {
public:
    static SensorData parse(const QString &line);
};
