#include "MapRasterizer_Metrics.h"

QString OsmAnd::MapRasterizer_Metrics::Metric_rasterize::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("elapsedTime = %1s")).arg(elapsedTime);

    return output;
}
