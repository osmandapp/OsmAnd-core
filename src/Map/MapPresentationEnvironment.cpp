#include "MapPresentationEnvironment.h"
#include "MapPresentationEnvironment_P.h"

#include "MapStyleBuiltinValueDefinitions.h"

OsmAnd::MapPresentationEnvironment::MapPresentationEnvironment(
    const std::shared_ptr<const IMapStyle>& mapStyle_,
    const float displayDensityFactor_ /*= 1.0f*/,
    const float mapScaleFactor_ /*= 1.0f*/,
    const float symbolsScaleFactor_ /*= 1.0f*/,
    const QString& localeLanguageId_ /*= QLatin1String("en")*/,
    const LanguagePreference languagePreference_ /*= LanguagePreference::LocalizedOrNative*/,
    const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_ /*= nullptr*/)
    : _p(new MapPresentationEnvironment_P(this))
    , styleBuiltinValueDefs(MapStyleBuiltinValueDefinitions::get())
    , mapStyle(mapStyle_)
    , displayDensityFactor(displayDensityFactor_)
    , mapScaleFactor(mapScaleFactor_)
    , symbolsScaleFactor(symbolsScaleFactor_)
    , localeLanguageId(localeLanguageId_)
    , languagePreference(languagePreference_)
    , externalResourcesProvider(externalResourcesProvider_)
{
    _p->initialize();
}

OsmAnd::MapPresentationEnvironment::~MapPresentationEnvironment()
{
}

QHash< OsmAnd::IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue >
OsmAnd::MapPresentationEnvironment::getSettings() const
{
    return _p->getSettings();
}

void OsmAnd::MapPresentationEnvironment::setSettings(
    const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings)
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

bool OsmAnd::MapPresentationEnvironment::obtainShader(
    const QString& name,
    sk_sp<const SkImage>& outShader) const
{
    return _p->obtainShader(name, outShader);
}

bool OsmAnd::MapPresentationEnvironment::obtainMapIcon(
    const QString& name,
    sk_sp<const SkImage>& outIcon) const
{
    return _p->obtainMapIcon(name, outIcon);
}

bool OsmAnd::MapPresentationEnvironment::obtainTextShield(
    const QString& name,
    sk_sp<const SkImage>& outTextShield) const
{
    return _p->obtainTextShield(name, outTextShield);
}

bool OsmAnd::MapPresentationEnvironment::obtainIconShield(
    const QString& name,
    sk_sp<const SkImage>& outIconShield) const
{
    return _p->obtainIconShield(name, outIconShield);
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment::getDefaultBackgroundColor(
    const ZoomLevel zoom) const
{
    return _p->getDefaultBackgroundColor(zoom);
}

void OsmAnd::MapPresentationEnvironment::obtainShadowOptions(
    const ZoomLevel zoom,
    ShadowMode& mode,
    ColorARGB& color) const
{
    _p->obtainShadowOptions(zoom, mode, color);
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

float OsmAnd::MapPresentationEnvironment::getDefaultSymbolPathSpacing() const
{
    return _p->getDefaultSymbolPathSpacing();
}

float OsmAnd::MapPresentationEnvironment::getDefaultBlockPathSpacing() const
{
    return _p->getDefaultBlockPathSpacing();
}

float OsmAnd::MapPresentationEnvironment::getGlobalPathPadding() const
{
    return _p->getGlobalPathPadding();
}

OsmAnd::MapStubStyle OsmAnd::MapPresentationEnvironment::getDesiredStubsStyle() const
{
    return _p->getDesiredStubsStyle();
}

QString OsmAnd::MapPresentationEnvironment::getWeatherContourLevels(const QString& weatherType, const ZoomLevel zoom) const
{
    return _p->getWeatherContourLevels(weatherType, zoom);
}

QString OsmAnd::MapPresentationEnvironment::getWeatherContourTypes(const QString& weatherType, const ZoomLevel zoom) const
{
    return _p->getWeatherContourTypes(weatherType, zoom);
}

OsmAnd::ColorARGB OsmAnd::MapPresentationEnvironment::getTransportRouteColor(const bool nightMode, const QString& renderAttrName) const
{
    return _p->getTransportRouteColor(nightMode, renderAttrName);
}

QHash<QString, int> OsmAnd::MapPresentationEnvironment::getLineRenderingAttributes(const QString &renderAttrName) const
{
    return _p->getLineRenderingAttributes(renderAttrName);
}

QHash<QString, int> OsmAnd::MapPresentationEnvironment::getGpxColors() const
{
    return _p->getGpxColors();
}

QHash<QString, QList<int>> OsmAnd::MapPresentationEnvironment::getGpxWidth() const
{
    return _p->getGpxWidth();
}

QPair<QString, uint32_t> OsmAnd::MapPresentationEnvironment::getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const
{
    return _p->getRoadRenderingAttributes(renderAttrName, additionalSettings);
}


