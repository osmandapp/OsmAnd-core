#include "IMapRenderer_Metrics.h"

QString OsmAnd::IMapRenderer_Metrics::Metric_update::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(PRINT_METRIC_FIELD);

    return output;
}

QString OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(PRINT_METRIC_FIELD);

    return output;
}

QString OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(PRINT_METRIC_FIELD);

    return output;
}
