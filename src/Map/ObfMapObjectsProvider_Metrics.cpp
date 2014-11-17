#include "ObfMapObjectsProvider_Metrics.h"

OsmAnd::ObfMapObjectsProvider_Metrics::Metric_obtainData::Metric_obtainData()
{
    reset();
}

OsmAnd::ObfMapObjectsProvider_Metrics::Metric_obtainData::~Metric_obtainData()
{
}

void OsmAnd::ObfMapObjectsProvider_Metrics::Metric_obtainData::reset()
{
    OsmAnd__ObfMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::ObfMapObjectsProvider_Metrics::Metric_obtainData::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__ObfMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(PRINT_METRIC_FIELD);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
