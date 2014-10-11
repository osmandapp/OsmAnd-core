#include "AtlasMapRenderer_Metrics.h"

QString OsmAnd::AtlasMapRenderer_Metrics::Metric_renderFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output = IMapRenderer_Metrics::Metric_renderFrame::toString(prefix);

    OsmAnd__AtlasMapRenderer_Metrics__Metric_renderFrame__FIELDS(PRINT_METRIC_FIELD);

    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/billboard-symbol-render = %1ms")).arg((elapsedTimeForBillboardSymbolsRendering / static_cast<float>(billboardSymbolsRendered)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/on-path-symbol-render = %1ms")).arg((elapsedTimeForOnPathSymbolsRendering / static_cast<float>(onPathSymbolsRendered)) * 1000.0f);
    output += QLatin1String("\n") + prefix + QString(QLatin1String("~time/on-surface-symbol-render = %1ms")).arg((elapsedTimeForOnSurfaceSymbolsRendering / static_cast<float>(onSurfaceSymbolsRendered)) * 1000.0f);

    return output;
}
