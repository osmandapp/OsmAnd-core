#include "MapPresentationEnvironment.h"
#include "MapPresentationEnvironment_P.h"

#include "MapStyleBuiltinValueDefinitions.h"

OsmAnd::MapPresentationEnvironment::MapPresentationEnvironment(
    const std::shared_ptr<const ResolvedMapStyle>& resolvedStyle_,
    const float displayDensityFactor_ /*= 1.0f*/,
    const QString& localeLanguageId_ /*= QLatin1String("en")*/,
    const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_ /*= nullptr*/)
    : _p(new MapPresentationEnvironment_P(this))
    , styleBuiltinValueDefs(MapStyleBuiltinValueDefinitions::get())
    , resolvedStyle(resolvedStyle_)
    , displayDensityFactor(displayDensityFactor_)
    , localeLanguageId(localeLanguageId_)
    , externalResourcesProvider(externalResourcesProvider_)
    , dummyMapSection(_p->dummyMapSection)
{
    _p->initialize();
}

OsmAnd::MapPresentationEnvironment::~MapPresentationEnvironment()
{
}

QHash< OsmAnd::ResolvedMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue > OsmAnd::MapPresentationEnvironment::getSettings() const
{
    return _p->getSettings();
}

void OsmAnd::MapPresentationEnvironment::setSettings(const QHash< OsmAnd::ResolvedMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings)
{
    _p->setSettings(newSettings);
}

void OsmAnd::MapPresentationEnvironment::setSettings(const QHash< QString, QString >& newSettings)
{
    _p->setSettings(newSettings);
}

void OsmAnd::MapPresentationEnvironment::applyTo(MapStyleEvaluator& evaluator) const
{
    _p->applyTo(evaluator);
}

bool OsmAnd::MapPresentationEnvironment::obtainShaderBitmap(const QString& name, std::shared_ptr<const SkBitmap>& outShaderBitmap) const
{
    return _p->obtainShaderBitmap(name, outShaderBitmap);
}

bool OsmAnd::MapPresentationEnvironment::obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const
{
    return _p->obtainMapIcon(name, outIcon);
}

bool OsmAnd::MapPresentationEnvironment::obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outTextShield) const
{
    return _p->obtainTextShield(name, outTextShield);
}

bool OsmAnd::MapPresentationEnvironment::obtainIconShield(const QString& name, std::shared_ptr<const SkBitmap>& outIconShield) const
{
    return _p->obtainIconShield(name, outIconShield);
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment::getDefaultBackgroundColor(const ZoomLevel zoom) const
{
    return _p->getDefaultBackgroundColor(zoom);
}

void OsmAnd::MapPresentationEnvironment::obtainShadowRenderingOptions(const ZoomLevel zoom, int& mode, ColorARGB& color) const
{
    _p->obtainShadowRenderingOptions(zoom, mode, color);
}

double OsmAnd::MapPresentationEnvironment::getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const
{
    return _p->getPolygonAreaMinimalThreshold(zoom);
}

unsigned int OsmAnd::MapPresentationEnvironment::getRoadDensityZoomTile(const ZoomLevel zoom) const
{
    return _p->getRoadDensityZoomTile(zoom);
}

unsigned int OsmAnd::MapPresentationEnvironment::getRoadsDensityLimitPerTile(const ZoomLevel zoom) const
{
    return _p->getRoadsDensityLimitPerTile(zoom);
}
