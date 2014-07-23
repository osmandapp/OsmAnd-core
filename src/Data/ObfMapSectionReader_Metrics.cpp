#include "ObfMapSectionReader_Metrics.h"

#include "QtExtensions.h"

QString OsmAnd::ObfMapSectionReader_Metrics::Metric_loadMapObjects::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("visitedLevels = %1\n")).arg(visitedLevels);
    output += prefix + QString(QLatin1String("acceptedLevels = %1\n")).arg(acceptedLevels);
    output += prefix + QString(QLatin1String("elapsedTimeForLevelsBbox = %1s\n")).arg(elapsedTimeForLevelsBbox);
    output += prefix + QString(QLatin1String("visitedNodes = %1\n")).arg(visitedNodes);
    output += prefix + QString(QLatin1String("acceptedNodes = %1\n")).arg(acceptedNodes);
    output += prefix + QString(QLatin1String("elapsedTimeForNodesBbox = %1s\n")).arg(elapsedTimeForNodesBbox);
    output += prefix + QString(QLatin1String("elapsedTimeForNodes = %1s\n")).arg(elapsedTimeForNodes);
    output += prefix + QString(QLatin1String("mapObjectsBlocksRead = %1\n")).arg(mapObjectsBlocksRead);
    output += prefix + QString(QLatin1String("mapObjectsBlocksReferenced = %1\n")).arg(mapObjectsBlocksReferenced);
    output += prefix + QString(QLatin1String("visitedMapObjects = %1\n")).arg(visitedMapObjects);
    output += prefix + QString(QLatin1String("acceptedMapObjects = %1\n")).arg(acceptedMapObjects);
    output += prefix + QString(QLatin1String("elapsedTimeForMapObjectsBlocks = %1s\n")).arg(elapsedTimeForMapObjectsBlocks);
    output += prefix + QString(QLatin1String("elapsedTimeForOnlyVisitedMapObjects = %1s\n")).arg(elapsedTimeForOnlyVisitedMapObjects);
    output += prefix + QString(QLatin1String("elapsedTimeForOnlyAcceptedMapObjects = %1s\n")).arg(elapsedTimeForOnlyAcceptedMapObjects);
    output += prefix + QString(QLatin1String("elapsedTimeForMapObjectsBbox = %1s\n")).arg(elapsedTimeForMapObjectsBbox);
    output += prefix + QString(QLatin1String("elapsedTimeForSkippedMapObjectsPoints = %1s\n")).arg(elapsedTimeForSkippedMapObjectsPoints);
    output += prefix + QString(QLatin1String("skippedMapObjectsPoints = %1\n")).arg(skippedMapObjectsPoints);
    output += prefix + QString(QLatin1String("elapsedTimeForNotSkippedMapObjectsPoints = %1s\n")).arg(elapsedTimeForNotSkippedMapObjectsPoints);
    output += prefix + QString(QLatin1String("notSkippedMapObjectsPoints = %1\n")).arg(notSkippedMapObjectsPoints);

    return output;
}
