#include "BinaryMapDataProvider_Metrics.h"

QString OsmAnd::BinaryMapDataProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += loadMapObjectsMetric.toString(prefix + QLatin1String("loadMapObjectsMetric:")) + QLatin1String("\n");
    OsmAnd__BinaryMapDataProvider_Metrics__Metric_obtainData__FIELDS(PRINT_METRIC_FIELD);

    return output;
}
