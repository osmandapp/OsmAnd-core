#include "MapPresentationEnvironment.h"
#include "MapPresentationEnvironment_P.h"

#include "MapStyleBuiltinValueDefinitions.h"

OsmAnd::MapPresentationEnvironment::MapPresentationEnvironment(
    const std::shared_ptr<const IMapStyle>& mapStyle_,
    const float displayDensityFactor_ /*= 1.0f*/,
    const float mapScaleFactor_ /*= 1.0f*/,
    const float symbolsScaleFactor_ /*= 1.0f*/,
    const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_ /*= nullptr*/,
    const QSet<QString> disabledAttributes /*= QSet<QString>()*/)
    : _p(new MapPresentationEnvironment_P(this))
    , styleBuiltinValueDefs(MapStyleBuiltinValueDefinitions::get())
    , mapStyle(mapStyle_)
    , displayDensityFactor(displayDensityFactor_)
    , mapScaleFactor(mapScaleFactor_)
    , symbolsScaleFactor(symbolsScaleFactor_)
    , externalResourcesProvider(externalResourcesProvider_)
    , disabledAttributes(disabledAttributes)
{
    _p->initialize();
}

OsmAnd::MapPresentationEnvironment::~MapPresentationEnvironment()
{
}

QString OsmAnd::MapPresentationEnvironment::getLocaleLanguageId() const
{
    return _p->getLocaleLanguageId();
}

void OsmAnd::MapPresentationEnvironment::setLocaleLanguageId(const QString& localeLanguageId)
{
    _p->setLocaleLanguageId(localeLanguageId);
}

OsmAnd::MapPresentationEnvironment::LanguagePreference OsmAnd::MapPresentationEnvironment::getLanguagePreference() const
{
    return _p->getLanguagePreference();
}

void OsmAnd::MapPresentationEnvironment::setLanguagePreference(const LanguagePreference languagePreference)
{
    _p->setLanguagePreference(languagePreference);
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

std::shared_ptr<const OsmAnd::LayeredIconData> OsmAnd::MapPresentationEnvironment::getLayeredIconData(
    const QString& tag,
    const QString& value,
    const ZoomLevel zoom,
    const int textLength) const
{
    return _p->getLayeredIconData(tag, value, zoom, textLength, nullptr);
}

std::shared_ptr<const OsmAnd::LayeredIconData> OsmAnd::MapPresentationEnvironment::getLayeredIconData(
    const QString& tag,
    const QString& value,
    const ZoomLevel zoom,
    const int textLength,
    const QString& additional) const
{
    return _p->getLayeredIconData(tag, value ,zoom, textLength, &additional);
}

std::shared_ptr<const OsmAnd::IconData> OsmAnd::MapPresentationEnvironment::getIconData(const QString& name) const
{
    return _p->getIconData(name);
}

std::shared_ptr<const OsmAnd::IconData> OsmAnd::MapPresentationEnvironment::getMapIconData(const QString& name) const
{
    return _p->getMapIconData(name);
}

std::shared_ptr<const OsmAnd::IconData> OsmAnd::MapPresentationEnvironment::getShaderOrShieldData(const QString& name) const
{
    return _p->getShaderOrShieldData(name);
}

bool OsmAnd::MapPresentationEnvironment::obtainIcon(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIcon) const
{
    return _p->obtainIcon(name, scale, outIcon);
}

bool OsmAnd::MapPresentationEnvironment::obtainMapIcon(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outIcon) const
{
    return _p->obtainMapIcon(name, scale, outIcon);
}

bool OsmAnd::MapPresentationEnvironment::obtainShaderOrShield(
    const QString& name,
    const float scale,
    sk_sp<const SkImage>& outTextShield) const
{
    return _p->obtainShaderOrShield(name, scale, outTextShield);
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

OsmAnd::FColorARGB OsmAnd::MapPresentationEnvironment::get3DBuildingsColor() const
{
    return _p->get3DBuildingsColor();
}

float OsmAnd::MapPresentationEnvironment::getDefault3DBuildingHeight() const
{
    return _p->getDefault3DBuildingHeight();
}

float OsmAnd::MapPresentationEnvironment::getDefault3DBuildingLevelHeight() const
{
    return _p->getDefault3DBuildingLevelHeight();
}

bool OsmAnd::MapPresentationEnvironment::getUseDefaultBuildingColor() const
{
    return _p->getUseDefaultBuildingColor();
}

bool OsmAnd::MapPresentationEnvironment::getShow3DBuildingParts() const
{
    return _p->getShow3DBuildingParts();
}

bool OsmAnd::MapPresentationEnvironment::getShow3DBuildingOutline() const
{
    return _p->getShow3DBuildingOutline();
}

bool OsmAnd::MapPresentationEnvironment::getColoredBuildings() const
{
    return _p->getColoredBuildings();
}


