#include "MapRasterizer_Metrics.h"

OsmAnd::MapRasterizer_Metrics::Metric_rasterize::Metric_rasterize()
{
    reset();
}

OsmAnd::MapRasterizer_Metrics::Metric_rasterize::~Metric_rasterize()
{
}

void OsmAnd::MapRasterizer_Metrics::Metric_rasterize::reset()
{
    OsmAnd__MapRasterizer_Metrics__Metric_rasterize__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::MapRasterizer_Metrics::Metric_rasterize::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapRasterizer_Metrics__Metric_rasterize__FIELDS(PRINT_METRIC_FIELD);
    output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
