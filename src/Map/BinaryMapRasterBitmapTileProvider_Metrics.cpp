#include "BinaryMapRasterBitmapTileProvider_Metrics.h"

QString OsmAnd::BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += obtainBinaryMapPrimitivesMetric.toString(prefix + QLatin1String("obtainBinaryMapPrimitivesMetric:")) + QLatin1String("\n");
    output += rasterizeMetric.toString(prefix + QLatin1String("rasterizeMetric:")) + QLatin1String("\n");
    OsmAnd__BinaryMapRasterBitmapTileProvider_Metrics__Metric_obtainData__FIELDS(PRINT_METRIC_FIELD);

    return output;
}
