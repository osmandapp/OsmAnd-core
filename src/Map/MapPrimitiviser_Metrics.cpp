#include "MapPrimitiviser_Metrics.h"

OsmAnd::MapPrimitiviser_Metrics::Metric_primitivise::Metric_primitivise()
{
    reset();
}

void OsmAnd::MapPrimitiviser_Metrics::Metric_primitivise::reset()
{
    OsmAnd__MapPrimitiviser_Metrics__Metric_primitivise__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::MapPrimitiviser_Metrics::Metric_primitivise::toString(
    const bool shortFormat /*= false*/,
    const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapPrimitiviser_Metrics__Metric_primitivise__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-order = %1ms"))
        .arg((elapsedTimeForOrderEvaluation * 1000.0f / static_cast<float>(orderEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-polygon = %1ms"))
        .arg((elapsedTimeForPolygonEvaluation * 1000.0f / static_cast<float>(polygonEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-polyline = %1ms"))
        .arg((elapsedTimeForPolylineEvaluation * 1000.0f / static_cast<float>(polylineEvaluations)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/1k-points = %1ms"))
        .arg((elapsedTimeForPointEvaluation * 1000.0f / static_cast<float>(pointEvaluations)) * 1000.0f);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}

OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects::Metric_primitiviseAllMapObjects()
{
    reset();
}

void OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects::reset()
{
    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseAllMapObjects__FIELDS(RESET_METRIC_FIELD);

    Metric_primitivise::reset();
}

QString OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects::toString(
    const bool shortFormat /*= false*/,
    const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseAllMapObjects__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + Metric_primitivise::toString(shortFormat, prefix);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}

OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface::Metric_primitiviseWithoutSurface()
{
    reset();
}

void OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface::reset()
{
    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithoutSurface__FIELDS(RESET_METRIC_FIELD);

    Metric_primitivise::reset();
}

QString OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface::toString(
    const bool shortFormat /*= false*/,
    const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithoutSurface__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + Metric_primitivise::toString(shortFormat, prefix);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}

OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithSurface::Metric_primitiviseWithSurface()
{
    reset();
}

void OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithSurface::reset()
{
    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithSurface__FIELDS(RESET_METRIC_FIELD);

    Metric_primitiviseWithoutSurface::reset();
}

QString OsmAnd::MapPrimitiviser_Metrics::Metric_primitiviseWithSurface::toString(
    const bool shortFormat /*= false*/,
    const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithSurface__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + Metric_primitiviseWithoutSurface::toString(shortFormat, prefix);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
