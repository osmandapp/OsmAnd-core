#include "BinaryMapPrimitivesProvider_Metrics.h"

QString OsmAnd::BinaryMapPrimitivesProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += obtainBinaryMapDataMetric.toString(prefix) + QLatin1String("\n");
    output += primitiviseMetric.toString(prefix);

    return output;
}
