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
    setVisualizationColorMapPreset(ColorMapPreset::GrayscaleHillshade);
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::resetVisualizationColorMap()
{
    for (auto& colorMapEntry : visualizationColorMap)
    {
        colorMapEntry = { 0.0f, FColorRGBA() };
    }

    return *this;
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::setVisualizationColorMapPreset(
    ColorMapPreset colorMapPreset)
{
    resetVisualizationColorMap();

    switch (colorMapPreset)
    {
        case ColorMapPreset::GrayscaleHillshade:
            visualizationColorMap[0] = {   0.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 1.0f) };
            visualizationColorMap[1] = { 255.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.0f) };
            break;
        case ColorMapPreset::GrayscaleSlopeDegrees:
            visualizationColorMap[0] = {   0.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.0f) };
            visualizationColorMap[1] = {  90.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 1.0f) };
            break;
        case ColorMapPreset::TerrainSlopeDegrees:
            visualizationColorMap[0] = {   0.00f, FColorRGB( 74.0f / 255.0f, 165.0f / 255.0f,  61.0f / 255.0f) };
            visualizationColorMap[1] = {   7.00f, FColorRGB(117.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f) };
            visualizationColorMap[2] = {  15.07f, FColorRGB(167.0f / 255.0f, 220.0f / 255.0f, 145.0f / 255.0f) };
            visualizationColorMap[3] = {  35.33f, FColorRGB(245.0f / 255.0f, 211.0f / 255.0f, 163.0f / 255.0f) };
            visualizationColorMap[4] = {  43.85f, FColorRGB(229.0f / 255.0f, 149.0f / 255.0f, 111.0f / 255.0f) };
            visualizationColorMap[5] = {  50.33f, FColorRGB(235.0f / 255.0f, 178.0f / 255.0f, 152.0f / 255.0f) };
            visualizationColorMap[6] = {  55.66f, FColorRGB(244.0f / 255.0f, 216.0f / 255.0f, 201.0f / 255.0f) };
            visualizationColorMap[7] = {  69.00f, FColorRGB(251.0f / 255.0f, 247.0f / 255.0f, 240.0f / 255.0f) };
            break;
    }

    return *this;
}

bool OsmAnd::ElevationConfiguration::isValid() const
{
    return (visualizationStyle == VisualizationStyle::None
            && slopeAlgorithm == SlopeAlgorithm::None)
        || ((visualizationStyle == VisualizationStyle::SlopeDegrees
                || visualizationStyle == VisualizationStyle::SlopePercents)
            && slopeAlgorithm != SlopeAlgorithm::None)
        || ((visualizationStyle == VisualizationStyle::HillshadeTraditional
                || visualizationStyle == VisualizationStyle::HillshadeIgor
                || visualizationStyle == VisualizationStyle::HillshadeCombined
                || visualizationStyle == VisualizationStyle::HillshadeMultidirectional)
            && slopeAlgorithm != SlopeAlgorithm::None);
}

OsmAnd::FogConfiguration::FogConfiguration()
    : distanceToFog(400.0f)
    , originFactor(0.36f)
    , heightOriginFactor(0.05f)
    , density(1.9f)
    , color(0.9215686f, 0.9058824f, 0.8941176f)
{
}
