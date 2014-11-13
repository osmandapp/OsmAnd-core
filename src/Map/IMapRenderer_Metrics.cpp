#include "IMapRenderer_Metrics.h"

OsmAnd::IMapRenderer_Metrics::Metric_update::Metric_update()
{
    reset();
}

OsmAnd::IMapRenderer_Metrics::Metric_update::~Metric_update()
{
}

void OsmAnd::IMapRenderer_Metrics::Metric_update::reset()
{
    OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::IMapRenderer_Metrics::Metric_update::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(PRINT_METRIC_FIELD);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}

OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::Metric_prepareFrame()
{
    reset();
}

OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::~Metric_prepareFrame()
{
}

void OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::reset()
{
    OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(PRINT_METRIC_FIELD);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}

OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::Metric_renderFrame()
{
    reset();
}

OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::~Metric_renderFrame()
{
}

void OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::reset()
{
    OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(RESET_METRIC_FIELD);

    Metric::reset();
}

QString OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::toString(const bool shortFormat /*= false*/, const QString& prefix /*= QString::null*/) const
{
    QString output;

    OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(PRINT_METRIC_FIELD);
    const auto submetricsString = Metric::toString(shortFormat, prefix);
    if (!submetricsString.isEmpty())
        output += QLatin1String("\n") + Metric::toString(shortFormat, prefix);

    return output;
}
