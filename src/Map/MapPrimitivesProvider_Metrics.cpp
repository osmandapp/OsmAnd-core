#include "MapPrimitivesProvider_Metrics.h"

OsmAnd::MapPrimitivesProvider_Metrics::Metric_obtainData::Metric_obtainData()
{
    reset();
}

OsmAnd::MapPrimitivesProvider_Metrics::Metric_obtainData::~Metric_obtainData()
{
}

void OsmAnd::MapPrimitivesProvider_Metrics::Metric_obtainData::reset()
{
    OsmAnd__MapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);
    
    Metric::reset();
}

QString OsmAnd::MapPrimitivesProvider_Metrics::Metric_obtainData::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(PRINT_METRIC_FIELD);
    output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
