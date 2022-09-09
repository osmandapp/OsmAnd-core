#include "MapRendererState.h"

OsmAnd::MapRendererState::MapRendererState()
    : fieldOfView(16.5f)
    , skyColor(ColorRGB(140, 190, 214))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(0, 0)
    , zoomLevel(MinZoomLevel)
    , visualZoom(1.0f)
    , visualZoomShift(0.0f)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , metersPerPixel(1.0)
{
}

OsmAnd::MapRendererState::~MapRendererState()
{
}

OsmAnd::MapState OsmAnd::MapRendererState::getMapState() const
{
    MapState mapState;

    mapState.fieldOfView = fieldOfView;
    mapState.skyColor = skyColor;
    mapState.azimuth = azimuth;
    mapState.elevationAngle = elevationAngle;
    mapState.target31 = target31;
    mapState.fixedPixel = fixedPixel;
    mapState.fixedLocation31 = fixedLocation31;
    mapState.zoomLevel = zoomLevel;
    mapState.visualZoom = visualZoom;
    mapState.visualZoomShift = visualZoomShift;
    mapState.stubsStyle = stubsStyle;
    mapState.backgroundColor = backgroundColor;
    
    mapState.metersPerPixel = metersPerPixel;
    mapState.visibleBBox31 = visibleBBox31;

    return mapState;
}

OsmAnd::MapState::MapState()
    : fieldOfView(16.5f)
    , skyColor(ColorRGB(140, 190, 214))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , fixedPixel(-1, -1)
    , fixedLocation31(0, 0)
    , zoomLevel(MinZoomLevel)
    , visualZoom(1.0f)
    , visualZoomShift(0.0f)
    , stubsStyle(MapStubStyle::Light)
    , backgroundColor(ColorRGB(0xf1, 0xee, 0xe8))
    , metersPerPixel(1.0)
{
}

OsmAnd::MapState::~MapState()
{
}
