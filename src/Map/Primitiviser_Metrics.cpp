#include "Primitiviser_Metrics.h"

QString OsmAnd::Primitiviser_Metrics::Metric_primitivise::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("elapsedTimeForSortingObjects = %1s\n")).arg(elapsedTimeForSortingObjects);
    output += prefix + QString(QLatin1String("elapsedTimeForPolygonizingCoastlines = %1s\n")).arg(elapsedTimeForPolygonizingCoastlines);
    output += prefix + QString(QLatin1String("polygonizedCoastlines = %1\n")).arg(polygonizedCoastlines);
    output += prefix + QString(QLatin1String("elapsedTimeForObtainingPrimitivesFromDetailedmap = %1s\n")).arg(elapsedTimeForObtainingPrimitivesFromDetailedmap);
    output += prefix + QString(QLatin1String("elapsedTimeForObtainingPrimitivesFromBasemap = %1s\n")).arg(elapsedTimeForObtainingPrimitivesFromBasemap);
    output += prefix + QString(QLatin1String("elapsedTimeForObtainingPrimitivesFromCoastlines = %1s\n")).arg(elapsedTimeForObtainingPrimitivesFromCoastlines);
    output += prefix + QString(QLatin1String("elapsedTimeForFutureSharedPrimitivesGroups = %1s\n")).arg(elapsedTimeForFutureSharedPrimitivesGroups);
    output += prefix + QString(QLatin1String("elapsedTimeForOrderEvaluation = %1s\n")).arg(elapsedTimeForOrderEvaluation);
    output += prefix + QString(QLatin1String("orderEvaluations = %1\n")).arg(orderEvaluations);
    output += prefix + QString(QLatin1String("orderRejects = %1\n")).arg(orderRejects);
    output += prefix + QString(QLatin1String("~time/1k-order = %1ms\n")).arg((elapsedTimeForOrderEvaluation * 1000.0f / static_cast<float>(orderEvaluations)) * 1000.0f);
    output += prefix + QString(QLatin1String("elapsedTimeForPolygonEvaluation = %1s\n")).arg(elapsedTimeForPolygonEvaluation);
    output += prefix + QString(QLatin1String("polygonEvaluations = %1\n")).arg(polygonEvaluations);
    output += prefix + QString(QLatin1String("polygonRejects = %1\n")).arg(polygonRejects);
    output += prefix + QString(QLatin1String("polygonsRejectedByArea = %1\n")).arg(polygonsRejectedByArea);
    output += prefix + QString(QLatin1String("polygonPrimitives = %1\n")).arg(polygonPrimitives);
    output += prefix + QString(QLatin1String("~time/1k-polygon = %1ms\n")).arg((elapsedTimeForPolygonEvaluation * 1000.0f / static_cast<float>(polygonEvaluations)) * 1000.0f);
    output += prefix + QString(QLatin1String("elapsedTimeForPolylineEvaluation = %1s\n")).arg(elapsedTimeForPolylineEvaluation);
    output += prefix + QString(QLatin1String("polylineEvaluations = %1\n")).arg(polylineEvaluations);
    output += prefix + QString(QLatin1String("polylineRejects = %1\n")).arg(polylineRejects);
    output += prefix + QString(QLatin1String("polylinePrimitives = %1\n")).arg(polylinePrimitives);
    output += prefix + QString(QLatin1String("~time/1k-polyline = %1ms\n")).arg((elapsedTimeForPolylineEvaluation * 1000.0f / static_cast<float>(polylineEvaluations)) * 1000.0f);
    output += prefix + QString(QLatin1String("elapsedTimeForPointEvaluation = %1s\n")).arg(elapsedTimeForPointEvaluation);
    output += prefix + QString(QLatin1String("pointEvaluations = %1\n")).arg(pointEvaluations);
    output += prefix + QString(QLatin1String("pointRejects = %1\n")).arg(pointRejects);
    output += prefix + QString(QLatin1String("pointPrimitives = %1\n")).arg(pointPrimitives);
    output += prefix + QString(QLatin1String("~time/1k-points = %1ms\n")).arg((elapsedTimeForPointEvaluation * 1000.0f / static_cast<float>(pointEvaluations)) * 1000.0f);
    output += prefix + QString(QLatin1String("elapsedTimeForSortingAndFilteringPrimitives = %1s\n")).arg(elapsedTimeForSortingAndFilteringPrimitives);
    output += prefix + QString(QLatin1String("elapsedTimeForObtainingPrimitivesSymbols = %1s")).arg(elapsedTimeForObtainingPrimitivesSymbols);

    return output;
}
