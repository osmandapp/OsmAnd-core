#ifndef _OSMAND_CORE_MAP_RENDERER_TYPES_H_
#define _OSMAND_CORE_MAP_RENDERER_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Utilities.h>

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

    struct OSMAND_CORE_API SymbolSubsectionConfiguration Q_DECL_FINAL
    {
        SymbolSubsectionConfiguration();

        float opacityFactor;
#if !defined(SWIG)
        inline SymbolSubsectionConfiguration& setOpacityFactor(const float newOpacityFactor)
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

        inline bool operator==(const SymbolSubsectionConfiguration& r) const
        {
            return
                qFuzzyCompare(opacityFactor, r.opacityFactor);
        }

        inline bool operator!=(const SymbolSubsectionConfiguration& r) const
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

    struct OSMAND_CORE_API GridParameters Q_DECL_FINAL
    {
        float factorX1; // = 1 - EPSG:4326 longitude (radians)
        float factorX2; // = 1 - Universal Transverse Mercator easting coordinate (100 kilometers)
        float factorX3; // = 1 - Mercator X coordinate (100 map kilometers)
        float offsetX;
        float factorY1; // = 1 - EPSG:4326 latitude (radians)
        float factorY2; // = 1 - Universal Transverse Mercator northing coordinate (100 kilometers)
        float factorY3; // = 1 - Mercator Y coordinate (100 map kilometers)
        float offsetY;
        ZoomLevel minZoom;
        ZoomLevel maxZoomForFloat; // Maximum zoom level to use only FP coordinates in GLSL shaders
        ZoomLevel maxZoomForMixed; // Maximum zoom level to use FP coordinates in GLSL shaders
    };

    struct OSMAND_CORE_API GridConfiguration Q_DECL_FINAL
    {
        enum class Projection {
            WGS84 = 0,
            UTM = 1,
            Mercator = 2,
        };

        GridConfiguration();

        Projection primaryProjection;
#if !defined(SWIG)
        inline GridConfiguration& setPrimaryProjection(const Projection newProjection)
        {
            primaryProjection = newProjection;

            return *this;
        }
#endif // !defined(SWIG)

        Projection secondaryProjection;
#if !defined(SWIG)
        inline GridConfiguration& setSecondaryProjection(const Projection newProjection)
        {
            secondaryProjection = newProjection;

            return *this;
        }
#endif // !defined(SWIG)

        float primaryGap;
#if !defined(SWIG)
        inline GridConfiguration& setPrimaryGap(const float newGap)
        {
            primaryGap = newGap;

            return *this;
        }
#endif // !defined(SWIG)

        float secondaryGap;
#if !defined(SWIG)
        inline GridConfiguration& setSecondaryGap(const float newGap)
        {
            secondaryGap = newGap;

            return *this;
        }
#endif // !defined(SWIG)

        float primaryGranularity;
#if !defined(SWIG)
        inline GridConfiguration& setPrimaryGranularity(const float newGranularity)
        {
            primaryGranularity = newGranularity;

            return *this;
        }
#endif // !defined(SWIG)

        float secondaryGranularity;
#if !defined(SWIG)
        inline GridConfiguration& setSecondaryGranularity(const float newGranularity)
        {
            secondaryGranularity = newGranularity;

            return *this;
        }
#endif // !defined(SWIG)

        float primaryThickness;
#if !defined(SWIG)
        inline GridConfiguration& setPrimaryThickness(const float newThickness)
        {
            primaryThickness = newThickness;

            return *this;
        }
#endif // !defined(SWIG)

        float secondaryThickness;
#if !defined(SWIG)
        inline GridConfiguration& setSecondaryThickness(const float newThickness)
        {
            secondaryThickness = newThickness;

            return *this;
        }
#endif // !defined(SWIG)

        FColorARGB primaryColor;
#if !defined(SWIG)
        inline GridConfiguration& setPrimaryColor(FColorARGB newColor)
        {
            primaryColor = newColor;

            return *this;
        }
#endif // !defined(SWIG)

        FColorARGB secondaryColor;
#if !defined(SWIG)
        inline GridConfiguration& setSecondaryColor(FColorARGB newColor)
        {
            secondaryColor = newColor;

            return *this;
        }
#endif // !defined(SWIG)

        GridParameters gridParameters[2];
#if !defined(SWIG)
        GridConfiguration& setProjectionParameters();
        GridConfiguration& setProjectionParameters(const int gridIndex, const Projection projection);
        double getPrimaryGridReference(const PointI& location31) const;
        double getSecondaryGridReference(const PointI& location31) const;
        PointD getPrimaryGridLocation(const PointI& location31, const double* referenceDeg = nullptr) const;
        PointD getSecondaryGridLocation(const PointI& location31, const double* referenceDeg = nullptr) const;
        double getLocationReference(const Projection projection, const PointI& location31) const;
        PointD projectLocation(
            const Projection projection, const PointI& location31, const double* referenceDeg = nullptr) const;
#endif // !defined(SWIG)

        bool isValid() const;

        inline bool operator==(const GridConfiguration& r) const
        {
            return
                primaryProjection == r.primaryProjection &&
                qFuzzyCompare(primaryGap, r.primaryGap) &&
                qFuzzyCompare(primaryGranularity, r.primaryGranularity) &&
                qFuzzyCompare(primaryThickness, r.primaryThickness) &&
                primaryColor == r.primaryColor &&
                secondaryProjection == r.secondaryProjection &&
                qFuzzyCompare(secondaryGap, r.secondaryGap) &&
                qFuzzyCompare(secondaryGranularity, r.secondaryGranularity) &&
                qFuzzyCompare(secondaryThickness, r.secondaryThickness) &&
                secondaryColor == r.secondaryColor;
        }

        inline bool operator!=(const GridConfiguration& r) const
        {
            return
                primaryProjection != r.primaryProjection ||
                !qFuzzyCompare(primaryGap, r.primaryGap) ||
                !qFuzzyCompare(primaryGranularity, r.primaryGranularity) ||
                !qFuzzyCompare(primaryThickness, r.primaryThickness) ||
                primaryColor != r.primaryColor ||
                secondaryProjection != r.secondaryProjection ||
                !qFuzzyCompare(secondaryGap, r.secondaryGap) ||
                !qFuzzyCompare(secondaryGranularity, r.secondaryGranularity) ||
                !qFuzzyCompare(secondaryThickness, r.secondaryThickness) ||
                secondaryColor != r.secondaryColor;
        }
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TYPES_H_)
