#include "BinaryMapDataProvider_Metrics.h"

QString OsmAnd::BinaryMapDataProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += loadMapObjectsMetrics.toString(prefix) + QLatin1String("\n");
    output += prefix + QString(QLatin1String("elapsedTimeForObtainingObfInterface = %1s\n")).arg(elapsedTimeForObtainingObfInterface);
    output += prefix + QString(QLatin1String("elapsedTimeForObjectsFiltering = %1s\n")).arg(elapsedTimeForObjectsFiltering);
    output += prefix + QString(QLatin1String("elapsedTimeForRead = %1s\n")).arg(elapsedTimeForRead);
    output += prefix + QString(QLatin1String("elapsedTime = %1s\n")).arg(elapsedTime);
    output += prefix + QString(QLatin1String("objectsCount = %1\n")).arg(objectsCount);
    output += prefix + QString(QLatin1String("uniqueObjectsCount = %1\n")).arg(uniqueObjectsCount);
    output += prefix + QString(QLatin1String("sharedObjectsCount = %1")).arg(sharedObjectsCount);

    return output;
}
