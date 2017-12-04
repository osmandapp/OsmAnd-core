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

    struct ElevationDataConfiguration Q_DECL_FINAL
    {
        ElevationDataConfiguration()
            : scaleFactor(1.0f)
        {
        }

        float scaleFactor;
#if !defined(SWIG)
        inline ElevationDataConfiguration& setScaleFactor(const float newScaleFactor)
        {
            scaleFactor = newScaleFactor;

            return *this;
        }
#endif // !defined(SWIG)

        inline bool isValid() const
        {
            return true;
        }

        inline bool operator==(const ElevationDataConfiguration& r) const
        {
            return
                qFuzzyCompare(scaleFactor, r.scaleFactor);
        }

        inline bool operator!=(const ElevationDataConfiguration& r) const
        {
            return
                !qFuzzyCompare(scaleFactor, r.scaleFactor);
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
