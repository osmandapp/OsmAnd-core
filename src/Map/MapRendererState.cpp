#include "MapRendererState.h"

OsmAnd::MapRendererState::MapRendererState()
    : fieldOfView(16.5f)
    , skyColor(ColorRGB(140, 190, 214))
    , azimuth(0.0f)
    , elevationAngle(45.0f)
    , target31(1u << (ZoomLevel::MaxZoomLevel - 1), 1u << (ZoomLevel::MaxZoomLevel - 1))
    , zoomLevel(MinZoomLevel)
    , visualZoom(1.0f)
    , visualZoomShift(0.0f)
    , stubsStyle(MapStubStyle::Light)
{
}

OsmAnd::MapRendererState::~MapRendererState()
{
}
