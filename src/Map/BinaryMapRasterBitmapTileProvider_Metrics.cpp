#include "BinaryMapRasterBitmapTileProvider_Metrics.h"

QString OsmAnd::BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += obtainBinaryMapPrimitivesMetric.toString(prefix + QLatin1String("obtainBinaryMapPrimitivesMetric:")) + QLatin1String("\n");
    output += rasterizeMetric.toString(prefix + QLatin1String("rasterizeMetric:")) + QLatin1String("\n");
    output += prefix + QString(QLatin1String("elapsedTime = %1s\n")).arg(elapsedTime);

    return output;
}
