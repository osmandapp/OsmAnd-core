#include "ObfMapSectionReader_Metrics.h"

#include "QtExtensions.h"

QString OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__ObfMapSectionReader_Metrics__Metric_loadMapObjects__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-visited = %1ms")).arg((elapsedTimeForOnlyVisitedMapObjects * 1000.0f / static_cast<float>(visitedMapObjects - acceptedMapObjects)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-accepted = %1ms")).arg((elapsedTimeForOnlyAcceptedMapObjects * 1000.0f / static_cast<float>(acceptedMapObjects)) * 1000.0f);

    return output;
}
