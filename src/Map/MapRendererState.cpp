#include "MapRendererState.h"

OsmAnd::MapRendererState::MapRendererState()
    : fieldOfView(16.5f)
    , visibleDistance(3500)
    , detailedDistance(0.5f)
    , skyColor(ColorRGB(255, 255, 255))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedHeight(0.0f)
    , fixedZoomLevel(MinZoomLevel)
    , zoomLevel(MinZoomLevel)
    , visualZoom(1.0f)
    , visualZoomShift(0.0f)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , fogColor(ColorRGB(0xeb, 0xe7, 0xe4))
    , symbolsOpacity(1.0f)
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
    mapState.zoomLevel = zoomLevel;
    mapState.visualZoom = visualZoom;
    mapState.visualZoomShift = visualZoomShift;
    mapState.stubsStyle = stubsStyle;
    mapState.backgroundColor = backgroundColor;
    mapState.fogColor = fogColor;
    mapState.symbolsOpacity = symbolsOpacity;
    
    mapState.metersPerPixel = metersPerPixel;
    mapState.visibleBBox31 = visibleBBox31;
    mapState.visibleBBoxShifted = visibleBBoxShifted;
    mapState.hasElevationDataProvider = elevationDataProvider != nullptr;

    return mapState;
}

OsmAnd::MapState::MapState()
    : fieldOfView(16.5f)
    , visibleDistance(3500)
    , detailedDistance(0.5f)
    , skyColor(ColorRGB(255, 255, 255))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedHeight(0.0f)
    , fixedZoomLevel(MinZoomLevel)
    , zoomLevel(MinZoomLevel)
    , visualZoom(1.0f)
    , visualZoomShift(0.0f)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , fogColor(ColorRGB(0xeb, 0xe7, 0xe4))
    , symbolsOpacity(1.0f)
    , metersPerPixel(1.0)
    , hasElevationDataProvider(false)
{
}

OsmAnd::MapState::~MapState()
{
}
