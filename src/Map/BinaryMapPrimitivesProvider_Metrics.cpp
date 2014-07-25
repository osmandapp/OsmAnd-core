#include "BinaryMapPrimitivesProvider_Metrics.h"

QString OsmAnd::BinaryMapPrimitivesProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += obtainBinaryMapDataMetric.toString(prefix + QLatin1String("obtainBinaryMapDataMetric:")) + QLatin1String("\n");
    output += primitiviseMetric.toString(prefix + QLatin1String("primitiviseMetric:")) + QLatin1String("\n");
    output += prefix + QString(QLatin1String("elapsedTime = %1s")).arg(elapsedTime);

    return output;
}
