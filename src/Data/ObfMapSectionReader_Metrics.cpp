#include "ObfMapSectionReader_Metrics.h"

#include "QtExtensions.h"

QString OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::toString() const
{
    QString output;

    output += QString(QLatin1String("visitedLevels = %1\n")).arg(visitedLevels);
    output += QString(QLatin1String("acceptedLevels = %1\n")).arg(acceptedLevels);
    output += QString(QLatin1String("visitedNodes = %1\n")).arg(visitedNodes);
    output += QString(QLatin1String("acceptedNodes = %1\n")).arg(acceptedNodes);
    output += QString(QLatin1String("elapsedTimeForNodes = %1\n")).arg(elapsedTimeForNodes);
    output += QString(QLatin1String("mapObjectsBlocksRead = %1\n")).arg(mapObjectsBlocksRead);
    output += QString(QLatin1String("visitedMapObjects = %1\n")).arg(visitedMapObjects);
    output += QString(QLatin1String("acceptedMapObjects = %1\n")).arg(acceptedMapObjects);
    output += QString(QLatin1String("elapsedTimeForMapObjectsBlocks = %1\n")).arg(elapsedTimeForMapObjectsBlocks);
    output += QString(QLatin1String("elapsedTimeForOnlyVisitedMapObjects = %1\n")).arg(elapsedTimeForOnlyVisitedMapObjects);
    output += QString(QLatin1String("elapsedTimeForOnlyAcceptedMapObjects = %1\n")).arg(elapsedTimeForOnlyAcceptedMapObjects);

    return output.trimmed();
}
