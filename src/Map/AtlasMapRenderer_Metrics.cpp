#include "AtlasMapRenderer_Metrics.h"

QString OsmAnd::AtlasMapRenderer_Metrics::Metric_renderFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output = IMapRenderer_Metrics::Metric_renderFrame::toString(prefix);

    OsmAnd__AtlasMapRenderer_Metrics__Metric_renderFrame__FIELDS(PRINT_METRIC_FIELD);

    return output;
}
