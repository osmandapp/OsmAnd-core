#include "ObfMapSectionReader_Metrics.h"

OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::Metric_loadMapObjects()
{
    reset();
}

OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::~Metric_loadMapObjects()
{
}

void OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::reset()
{
    OsmAnd__ObfMapSectionReader_Metrics__Metric_loadMapObjects__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__ObfMapSectionReader_Metrics__Metric_loadMapObjects__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-visited = %1ms")).arg((elapsedTimeForOnlyVisitedMapObjects * 1000.0f / static_cast<float>(visitedMapObjects - acceptedMapObjects)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-accepted = %1ms")).arg((elapsedTimeForOnlyAcceptedMapObjects * 1000.0f / static_cast<float>(acceptedMapObjects)) * 1000.0f);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
