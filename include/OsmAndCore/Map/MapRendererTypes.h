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
    struct OSMAND_CORE_API MapLayerConfiguration Q_DECL_FINAL
    {
        MapLayerConfiguration();

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

    struct OSMAND_CORE_API ElevationConfiguration Q_DECL_FINAL
    {
        enum class SlopeAlgorithm {
            None = 0,
            ZevenbergenThorne = 4,
            Horn = 8,
        };

        enum class VisualizationStyle {
            None = 0,
            SlopeDegrees = 10,
            SlopePercents = 11,
            HillshadeTraditional = 20,
            HillshadeIgor = 21,
            HillshadeCombined = 22,
            HillshadeMultidirectional = 23,
        };

        enum class ColorMapPreset {
            GrayscaleHillshade,
            GrayscaleSlopeDegrees,
            TerrainSlopeDegrees,
        };

        enum {
            MaxColorMapEntries = 8,
        };

        ElevationConfiguration();

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

        float visualizationZ;
#if !defined(SWIG)
        inline ElevationConfiguration& setVisualizationZ(const float newVisualizationZ)
        {
            visualizationZ = newVisualizationZ;

            return *this;
        }
#endif // !defined(SWIG)

        float hillshadeSunAngle;
#if !defined(SWIG)
        inline ElevationConfiguration& setHillshadeSunAngle(const float newHillshadeSunAngle)
        {
            hillshadeSunAngle = newHillshadeSunAngle;

            return *this;
        }
#endif // !defined(SWIG)

        float hillshadeSunAzimuth;
#if !defined(SWIG)
        inline ElevationConfiguration& setHillshadeSunAzimuth(const float newHillshadeSunAzimuth)
        {
            hillshadeSunAzimuth = newHillshadeSunAzimuth;

            return *this;
        }
#endif // !defined(SWIG)

        std::array<std::pair<float, FColorRGBA>, MaxColorMapEntries> visualizationColorMap;
#if !defined(SWIG)
        ElevationConfiguration& resetVisualizationColorMap();
        ElevationConfiguration& setVisualizationColorMapPreset(ColorMapPreset colorMapPreset);
#endif // !defined(SWIG)

        float zScaleFactor;
#if !defined(SWIG)
        inline ElevationConfiguration& setZScaleFactor(const float newZScaleFactor)
        {
            zScaleFactor = newZScaleFactor;

            return *this;
        }
#endif // !defined(SWIG)

        bool isValid() const;

        inline bool operator==(const ElevationConfiguration& r) const
        {
            return
                qFuzzyCompare(dataScaleFactor, r.dataScaleFactor) &&
                slopeAlgorithm == r.slopeAlgorithm &&
                visualizationStyle == r.visualizationStyle &&
                qFuzzyCompare(visualizationAlpha, r.visualizationAlpha) &&
                qFuzzyCompare(visualizationZ, r.visualizationZ) &&
                visualizationColorMap == r.visualizationColorMap &&
                qFuzzyCompare(hillshadeSunAngle, r.hillshadeSunAngle) &&
                qFuzzyCompare(hillshadeSunAzimuth, r.hillshadeSunAzimuth) &&
                qFuzzyCompare(zScaleFactor, r.zScaleFactor);
        }

        inline bool operator!=(const ElevationConfiguration& r) const
        {
            return
                !qFuzzyCompare(dataScaleFactor, r.dataScaleFactor) ||
                slopeAlgorithm != r.slopeAlgorithm ||
                visualizationStyle != r.visualizationStyle ||
                !qFuzzyCompare(visualizationAlpha, r.visualizationAlpha) ||
                !qFuzzyCompare(visualizationZ, r.visualizationZ) ||
                visualizationColorMap != r.visualizationColorMap ||
                !qFuzzyCompare(hillshadeSunAngle, r.hillshadeSunAngle) ||
                !qFuzzyCompare(hillshadeSunAzimuth, r.hillshadeSunAzimuth) ||
                !qFuzzyCompare(zScaleFactor, r.zScaleFactor);
        }
    };

    struct OSMAND_CORE_API FogConfiguration Q_DECL_FINAL
    {
        FogConfiguration();

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
