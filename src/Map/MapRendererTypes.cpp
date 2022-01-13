#include "MapRendererTypes.h"
#include "MapRendererTypes_private.h"

OsmAnd::MapLayerConfiguration::MapLayerConfiguration()
	: opacityFactor(1.0f)
{
}

OsmAnd::ElevationConfiguration::ElevationConfiguration()
    : dataScaleFactor(1.0f)
    , slopeAlgorithm(SlopeAlgorithm::ZevenbergenThorne)
    , visualizationStyle(VisualizationStyle::HillshadeTraditional)
    , visualizationAlpha(1.0f)
    , visualizationZ(1.0f)
    , hillshadeSunAngle(45.0f)
    , hillshadeSunAzimuth(315.0f)
    , zScaleFactor(1.0f)
{
	resetVisualizationColorMap
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::resetVisualizationColorMap()
{
	visualizationColorMap[0] = {  0.0f, FColorRGBA() };

    return *this;
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::setVisualizationGrayscaleSlopeColorMap()
{
    visualizationColorMap[0] = {  0.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.0f) };
    visualizationColorMap[1] = { 90.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 1.0f) };
    visualizationColorMap[2] = {  0.0f, FColorRGBA() };

    return *this;
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::setVisualizationTerrainSlopeColorMap()
{
    visualizationColorMap[0] = {  0.00f, FColorRGB( 74.0f / 255.0f, 165.0f / 255.0f,  61.0f / 255.0f) };
    visualizationColorMap[1] = {  7.00f, FColorRGB(117.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f) };
    visualizationColorMap[2] = { 15.07f, FColorRGB(167.0f / 255.0f, 220.0f / 255.0f, 145.0f / 255.0f) };
    visualizationColorMap[3] = { 35.33f, FColorRGB(245.0f / 255.0f, 211.0f / 255.0f, 163.0f / 255.0f) };
    visualizationColorMap[4] = { 43.85f, FColorRGB(229.0f / 255.0f, 149.0f / 255.0f, 111.0f / 255.0f) };
    visualizationColorMap[5] = { 50.33f, FColorRGB(235.0f / 255.0f, 178.0f / 255.0f, 152.0f / 255.0f) };
    visualizationColorMap[6] = { 55.66f, FColorRGB(244.0f / 255.0f, 216.0f / 255.0f, 201.0f / 255.0f) };
    visualizationColorMap[7] = { 69.00f, FColorRGB(251.0f / 255.0f, 247.0f / 255.0f, 240.0f / 255.0f) };

    return *this;
}

OsmAnd::FogConfiguration::FogConfiguration()
    : distanceToFog(400.0f)
    , originFactor(0.36f)
    , heightOriginFactor(0.05f)
    , density(1.9f)
    , color(1.0f, 0.0f, 0.0f)
{
}
