#ifndef _OSMAND_CORE_MAP_RENDERER_TYPES_H_
#define _OSMAND_CORE_MAP_RENDERER_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/CommonSWIG.h>

namespace OsmAnd
{
    struct MapLayerConfiguration Q_DECL_FINAL
    {
        MapLayerConfiguration()
            : opacityFactor(1.0f)
        {
        }

        float opacityFactor;
#if !defined(SWIG)
        inline MapLayerConfiguration& setOpacityFactor(const float newOpacityFactor)
        {
            opacityFactor = newOpacityFactor;

            return *this;
        }
#endif // !defined(SWIG)

        inline bool isValid() const
        {
            return
                (opacityFactor >= 0.0f && opacityFactor <= 1.0f);
        }

        inline bool operator==(const MapLayerConfiguration& r) const
        {
            return
                qFuzzyCompare(opacityFactor, r.opacityFactor);
        }

        inline bool operator!=(const MapLayerConfiguration& r) const
        {
            return
                !qFuzzyCompare(opacityFactor, r.opacityFactor);
        }
    };

    struct ElevationConfiguration Q_DECL_FINAL
    {
        enum class SlopeAlgorithm {
            None = 0,
            ZevenbergenThorne = 4,
            Horn = 8,
        };

        enum class VisualizationStyle {
            None = 0,
            HillshadeTraditional = 11,
            HillshadeIgor = 12,
            HillshadeCombined = 13,
            HillshadeMultidirectional = 14,
        };

        ElevationConfiguration()
            : dataScaleFactor(1.0f)
            , slopeAlgorithm(SlopeAlgorithm::ZevenbergenThorne)
            , visualizationStyle(VisualizationStyle::HillshadeMultidirectional)
            , visualizationAlpha(1.0f)
            , zScaleFactor(1.0f)
        {
            setGrayscaleSlopeColorMap();
        }

        float dataScaleFactor;
#if !defined(SWIG)
        inline ElevationConfiguration& setDataScaleFactor(const float newDataScaleFactor)
        {
            dataScaleFactor = newDataScaleFactor;

            return *this;
        }
#endif // !defined(SWIG)

        SlopeAlgorithm slopeAlgorithm;
#if !defined(SWIG)
        inline ElevationConfiguration& setSlopeAlgorithm(const SlopeAlgorithm newSlopeAlgorithm)
        {
            slopeAlgorithm = newSlopeAlgorithm;

            return *this;
        }
#endif // !defined(SWIG)

        VisualizationStyle visualizationStyle;
#if !defined(SWIG)
        inline ElevationConfiguration& setVisualizationStyle(const VisualizationStyle newVisualizationStyle)
        {
            visualizationStyle = newVisualizationStyle;

            return *this;
        }
#endif // !defined(SWIG)

        float visualizationAlpha;
#if !defined(SWIG)
        inline ElevationConfiguration& setVisualizationAlpha(const float newVisualizationAlpha)
        {
            visualizationAlpha = newVisualizationAlpha;

            return *this;
        }
#endif // !defined(SWIG)

        std::array<std::pair<float, FColorRGB>, 8> colorMap;
#if !defined(SWIG)
        inline ElevationConfiguration& setGrayscaleSlopeColorMap()
        {
            colorMap[0] = {  0.0f, FColorRGB(1.0f, 1.0f, 1.0f) };
            colorMap[1] = { 90.0f, FColorRGB(0.0f, 0.0f, 0.0f) };
            colorMap[2] = {  0.0f, FColorRGB() };

            return *this;
        }

        inline ElevationConfiguration& setTerrainSlopeColorMap()
        {
            colorMap[0] = {  0.00f, FColorRGB( 74.0f / 255.0f, 165.0f / 255.0f,  61.0f / 255.0f) };
            colorMap[1] = {  7.00f, FColorRGB(117.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f) };
            colorMap[2] = { 15.07f, FColorRGB(167.0f / 255.0f, 220.0f / 255.0f, 145.0f / 255.0f) };
            colorMap[3] = { 35.33f, FColorRGB(245.0f / 255.0f, 211.0f / 255.0f, 163.0f / 255.0f) };
            colorMap[4] = { 43.85f, FColorRGB(229.0f / 255.0f, 149.0f / 255.0f, 111.0f / 255.0f) };
            colorMap[5] = { 50.33f, FColorRGB(235.0f / 255.0f, 178.0f / 255.0f, 152.0f / 255.0f) };
            colorMap[6] = { 55.66f, FColorRGB(244.0f / 255.0f, 216.0f / 255.0f, 201.0f / 255.0f) };
            colorMap[7] = { 69.00f, FColorRGB(251.0f / 255.0f, 247.0f / 255.0f, 240.0f / 255.0f) };

            return *this;
        }
#endif // !defined(SWIG)

        float zScaleFactor;
#if !defined(SWIG)
        inline ElevationConfiguration& setZScaleFactor(const float newZScaleFactor)
        {
            zScaleFactor = newZScaleFactor;

            return *this;
        }
#endif // !defined(SWIG)

        inline bool isValid() const
        {
            return (visualizationStyle == VisualizationStyle::None) ||
                ((visualizationStyle == VisualizationStyle::HillshadeTraditional ||
                    visualizationStyle == VisualizationStyle::HillshadeMultidirectional) && (
                        slopeAlgorithm != SlopeAlgorithm::None));
        }

        inline bool operator==(const ElevationConfiguration& r) const
        {
            return
                qFuzzyCompare(dataScaleFactor, r.dataScaleFactor) &&
                slopeAlgorithm == r.slopeAlgorithm &&
                visualizationStyle == r.visualizationStyle &&
                qFuzzyCompare(visualizationAlpha, r.visualizationAlpha) &&
                colorMap == r.colorMap &&
                qFuzzyCompare(zScaleFactor, r.zScaleFactor);
        }

        inline bool operator!=(const ElevationConfiguration& r) const
        {
            return
                !qFuzzyCompare(dataScaleFactor, r.dataScaleFactor) ||
                slopeAlgorithm != r.slopeAlgorithm ||
                visualizationStyle != r.visualizationStyle ||
                !qFuzzyCompare(visualizationAlpha, r.visualizationAlpha) ||
                colorMap != r.colorMap ||
                !qFuzzyCompare(zScaleFactor, r.zScaleFactor);
        }
    };

    struct FogConfiguration Q_DECL_FINAL
    {
        FogConfiguration()
            : distanceToFog(400.0f) //700
            , originFactor(0.36f)
            , heightOriginFactor(0.05f)
            , density(1.9f)
            , color(1.0f, 0.0f, 0.0f)
        {
        }

        float distanceToFog;
#if !defined(SWIG)
        inline FogConfiguration& setDistanceToFog(const float newDistanceToFog)
        {
            distanceToFog = newDistanceToFog;

            return *this;
        }
#endif // !defined(SWIG)

        float originFactor;
#if !defined(SWIG)
        inline FogConfiguration& setOriginFactor(const float newOriginFactor)
        {
            originFactor = newOriginFactor;

            return *this;
        }
#endif // !defined(SWIG)

        float heightOriginFactor;
#if !defined(SWIG)
        inline FogConfiguration& setHeightOriginFactor(const float newHeightOriginFactor)
        {
            heightOriginFactor = newHeightOriginFactor;

            return *this;
        }
#endif // !defined(SWIG)

        float density;
#if !defined(SWIG)
        inline FogConfiguration& setDensity(const float newDensity)
        {
            density = newDensity;

            return *this;
        }
#endif // !defined(SWIG)

        FColorRGB color;
#if !defined(SWIG)
        inline FogConfiguration& setColor(const FColorRGB newColor)
        {
            color = newColor;

            return *this;
        }
#endif // !defined(SWIG)

        inline bool isValid() const
        {
            return
                (distanceToFog > 0.0f) &&
                (originFactor > 0.0f && originFactor <= 1.0f) &&
                (heightOriginFactor > 0.0f && heightOriginFactor <= 1.0f) &&
                (density > 0.0f);
        }

        inline bool operator==(const FogConfiguration& r) const
        {
            return
                qFuzzyCompare(distanceToFog, r.distanceToFog) &&
                qFuzzyCompare(originFactor, r.originFactor) &&
                qFuzzyCompare(heightOriginFactor, r.heightOriginFactor) &&
                qFuzzyCompare(density, r.density) &&
                color == r.color;
        }

        inline bool operator!=(const FogConfiguration& r) const
        {
            return
                !qFuzzyCompare(distanceToFog, r.distanceToFog) ||
                !qFuzzyCompare(originFactor, r.originFactor) ||
                !qFuzzyCompare(heightOriginFactor, r.heightOriginFactor) ||
                !qFuzzyCompare(density, r.density) ||
                color != r.color;
        }
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TYPES_H_)
