#include "MapRendererState.h"
#include "IMapElevationDataProvider.h"

OsmAnd::MapRendererState::MapRendererState()
    : isChanged(true)
    , flip(false)
    , flatEarth(true)
    , fieldOfView(16.5f)
    , visibleDistance(3500.0f)
    , detailedDistance(0.5f)
    , skyColor(ColorRGB(255, 255, 255))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , elevationAngleToBe(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedHeight(0.0f)
    , fixedZoomLevel(ZoomLevel2)
    , aimPixel(-1, -1)
    , aimLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , aimHeight(0.0f)
    , aimZoomLevel(ZoomLevel2)
    , aimingActions(0)
    , zoomLevel(ZoomLevel2)
    , visualZoom(1.0f)
    , surfaceZoomLevel(ZoomLevel2)
    , surfaceVisualZoom(1.0f)
    , surfaceZoomLevelToBe(ZoomLevel2)
    , surfaceVisualZoomToBe(1.0f)
    , visualZoomShift(0.0f)
    , minZoomLimit(ZoomLevel2)
    , maxZoomLimit(ZoomLevel22)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , fogColor(ColorRGB(0xeb, 0xe7, 0xe4))
    , myLocationColor(ColorARGB(0x80, 0x80, 0x80, 0x80))
    , myLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , myLocationRadiusInMeters(0.0f)
    , myDirection(0.0f)
    , myDirectionRadius(0.0f)
    , symbolsOpacity(1.0f)
    , dateTime(0)
    , metersPerPixel(1.0)
{
}

OsmAnd::MapRendererState::~MapRendererState()
{
}

OsmAnd::MapState OsmAnd::MapRendererState::getMapState() const
{
    MapState mapState;

    mapState.windowSize = windowSize;
    mapState.viewport = viewport;
    mapState.flip = flip;
    mapState.flatEarth = flatEarth;
    mapState.fieldOfView = fieldOfView;
    mapState.visibleDistance = visibleDistance;
    mapState.detailedDistance = detailedDistance;
    mapState.skyColor = skyColor;
    mapState.azimuth = azimuth;
    mapState.elevationAngle = elevationAngle;
    mapState.target31 = target31;
    mapState.fixedPixel = fixedPixel;
    mapState.fixedLocation31 = fixedLocation31;
    mapState.fixedHeight = fixedHeight;
    mapState.fixedZoomLevel = fixedZoomLevel;
    mapState.aimPixel = aimPixel;
    mapState.aimLocation31 = aimLocation31;
    mapState.aimHeight = aimHeight;
    mapState.aimZoomLevel = aimZoomLevel;
    mapState.aimingActions = aimingActions;
    mapState.zoomLevel = zoomLevel;
    mapState.visualZoom = visualZoom;
    mapState.surfaceZoomLevel = surfaceZoomLevel;
    mapState.surfaceVisualZoom = surfaceVisualZoom;
    mapState.visualZoomShift = visualZoomShift;
    mapState.minZoomLimit = minZoomLimit;
    mapState.maxZoomLimit = maxZoomLimit;
    mapState.stubsStyle = stubsStyle;
    mapState.backgroundColor = backgroundColor;
    mapState.fogColor = fogColor;
    mapState.myLocationColor = myLocationColor;
    mapState.myLocation31 = myLocation31;
    mapState.myLocationRadiusInMeters = myLocationRadiusInMeters;
    mapState.myDirection = myDirection;
    mapState.myDirectionRadius = myDirectionRadius;
    mapState.symbolsOpacity = symbolsOpacity;
    mapState.dateTime = dateTime;
    
    mapState.metersPerPixel = metersPerPixel;
    mapState.visibleBBox31 = visibleBBox31;
    mapState.visibleBBoxShifted = visibleBBoxShifted;
    mapState.hasElevationDataProvider = elevationDataProvider != nullptr;
    mapState.hasElevationDataResources = elevationDataProvider != nullptr && elevationDataProvider->hasDataResources();

    return mapState;
}

OsmAnd::MapState::MapState()
    : fieldOfView(16.5f)
    , visibleDistance(3500.0f)
    , detailedDistance(0.5f)
    , skyColor(ColorRGB(255, 255, 255))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedHeight(0.0f)
    , fixedZoomLevel(ZoomLevel2)
    , aimPixel(-1, -1)
    , aimLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , aimHeight(0.0f)
    , aimZoomLevel(ZoomLevel2)
    , zoomLevel(ZoomLevel2)
    , visualZoom(1.0f)
    , surfaceZoomLevel(ZoomLevel2)
    , surfaceVisualZoom(1.0f)
    , visualZoomShift(0.0f)
    , minZoomLimit(ZoomLevel2)
    , maxZoomLimit(ZoomLevel22)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , fogColor(ColorRGB(0xeb, 0xe7, 0xe4))
    , myLocationColor(ColorARGB(0x80, 0x80, 0x80, 0x80))
    , myLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , myLocationRadiusInMeters(0.0f)
    , myDirection(0.0f)
    , myDirectionRadius(0.0f)
    , symbolsOpacity(1.0f)
    , dateTime(0)
    , metersPerPixel(1.0)
    , hasElevationDataProvider(false)
    , hasElevationDataResources(false)
{
}

OsmAnd::MapState::~MapState()
{
}

void OsmAnd::MapRendererState::getGridConfiguration(GridConfiguration* gridConfiguration_,
    ZoomLevel* zoomLevel_) const
{
    *gridConfiguration_ = gridConfiguration;
    *zoomLevel_ = surfaceZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapRendererState::getVisibleArea(AreaI* visibleBBoxShifted_, PointI* target31_) const
{
    *visibleBBoxShifted_ = visibleBBoxShifted;
    *target31_ = target31;
    return surfaceZoomLevel;
}
