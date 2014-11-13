#include "ObfRoutingSectionReader_Metrics.h"

OsmAnd::ObfRoutingSectionReader_Metrics::Metric_loadRoads::Metric_loadRoads()
{
    reset();
}

OsmAnd::ObfRoutingSectionReader_Metrics::Metric_loadRoads::~Metric_loadRoads()
{
}

void OsmAnd::ObfRoutingSectionReader_Metrics::Metric_loadRoads::reset()
{
    OsmAnd__ObfRoutingSectionReader_Metrics__Metric_loadRoads__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::ObfRoutingSectionReader_Metrics::Metric_loadRoads::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__ObfRoutingSectionReader_Metrics__Metric_loadRoads__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-visited = %1ms")).arg((elapsedTimeForOnlyVisitedRoads * 1000.0f / static_cast<float>(visitedRoads - acceptedRoads)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-only-accepted = %1ms")).arg((elapsedTimeForOnlyAcceptedRoads * 1000.0f / static_cast<float>(acceptedRoads)) * 1000.0f);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
