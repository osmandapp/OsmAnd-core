#include "ObfRoutingSectionReader_Metrics.h"

#include "QtExtensions.h"

QString OsmAnd::ObfRoutingSectionReader_Metrics::Metric_loadRoads::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__ObfRoutingSectionReader_Metrics__Metric_loadRoads__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-visited = %1ms")).arg((elapsedTimeForOnlyVisitedRoads * 1000.0f / static_cast<float>(visitedRoads - acceptedRoads)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-accepted = %1ms")).arg((elapsedTimeForOnlyAcceptedRoads * 1000.0f / static_cast<float>(acceptedRoads)) * 1000.0f);

    return output;
}
