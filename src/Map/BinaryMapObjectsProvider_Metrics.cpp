#include "BinaryMapObjectsProvider_Metrics.h"

OsmAnd::BinaryMapObjectsProvider_Metrics::Metric_obtainData::Metric_obtainData()
{
    reset();
}

OsmAnd::BinaryMapObjectsProvider_Metrics::Metric_obtainData::~Metric_obtainData()
{
}

void OsmAnd::BinaryMapObjectsProvider_Metrics::Metric_obtainData::reset()
{
    OsmAnd__BinaryMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::BinaryMapObjectsProvider_Metrics::Metric_obtainData::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__BinaryMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(PRINT_METRIC_FIELD);
    output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
