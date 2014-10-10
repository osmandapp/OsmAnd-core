#include "Primitiviser_Metrics.h"

QString OsmAnd::Primitiviser_Metrics::Metric_primitivise::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__Primitiviser_Metrics__Metric_primitivise__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-order = %1ms")).arg((elapsedTimeForOrderEvaluation * 1000.0f / static_cast<float>(orderEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-polygon = %1ms")).arg((elapsedTimeForPolygonEvaluation * 1000.0f / static_cast<float>(polygonEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-polyline = %1ms")).arg((elapsedTimeForPolylineEvaluation * 1000.0f / static_cast<float>(polylineEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-points = %1ms")).arg((elapsedTimeForPointEvaluation * 1000.0f / static_cast<float>(pointEvaluations)) * 1000.0f);
    
    return output;
}
