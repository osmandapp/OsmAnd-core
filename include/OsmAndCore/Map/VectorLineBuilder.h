#ifndef _OSMAND_CORE_VECTOR_LINE_BUILDER_H_
#define _OSMAND_CORE_VECTOR_LINE_BUILDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QHash>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Map/VectorLine.h>
#include <OsmAndCore/SingleSkImage.h>

namespace OsmAnd
{
    class VectorLinesCollection;

    class VectorLineBuilder_P;
    class OSMAND_CORE_API VectorLineBuilder
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLineBuilder);

    private:
        PrivateImplementation<VectorLineBuilder_P> _p;
    protected:
    public:
        VectorLineBuilder();
        virtual ~VectorLineBuilder();

        bool isHidden() const;
        VectorLineBuilder& setIsHidden(const bool hidden);

        // Get/set the distance (in meters) from which only part of the line should be drawn,
        float getStartingDistance() const;
        VectorLineBuilder& setStartingDistance(const float distanceInMeters);

        bool shouldShowArrows() const;
        VectorLineBuilder& setShouldShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        VectorLineBuilder& setApproximationEnabled(const bool enabled);

        int getLineId() const;
        VectorLineBuilder& setLineId(const int lineId);

        int getBaseOrder() const;
        VectorLineBuilder& setBaseOrder(const int baseOrder);

        // Get/set line width
        double getLineWidth() const;
        VectorLineBuilder& setLineWidth(const double width);

        // Get/set single solid color for path (when colorization scheme is 0)
        FColorARGB getFillColor() const;
        VectorLineBuilder& setFillColor(const FColorARGB fillColor);

        std::vector<double> getLineDash() const;
        VectorLineBuilder& setLineDash(const std::vector<double> dashPattern);

        // Get/set path points
        QVector<PointI> getPoints() const;
        VectorLineBuilder& setPoints(const QVector<PointI>& points);

        // Get/set elevation of path points (in meters from sea level) for 3D vector line
        QList<float> getHeights() const;
        VectorLineBuilder& setHeights(const QList<float>& heights);

        // Get/set color map (see get/set colorization scheme) for the path line
        QList<OsmAnd::FColorARGB> getColorizationMapping() const;
        OsmAnd::VectorLineBuilder& setColorizationMapping(const QList<OsmAnd::FColorARGB>& colorizationMapping);

        // Get/set color map (see get/set colorization scheme) for the outline
        // using near and far outline colors as filters (see get/set near and far outline colors)
        QList<OsmAnd::FColorARGB> getOutlineColorizationMapping() const;
        OsmAnd::VectorLineBuilder& setOutlineColorizationMapping(const QList<OsmAnd::FColorARGB>& colorizationMapping);

        // Get/set the colorization scheme value, which is:
        // 0 (NONE) - use one color (see get/set fill color),
        // 1 (GRADIENT) - use gradient color map (see get/set colorization mapping),
        // 2 (SOLID) - use solid colors for each segment from zero-based color map (see get/set colorization mapping)
        int getColorizationScheme() const;
        OsmAnd::VectorLineBuilder& setColorizationScheme(const int colorizationScheme);

        // Get/set width for outline, which is:
        // for 2D vector line - usual outline of separate colouring,
        // for 3D vector line - filler of the same color under the line
        double getOutlineWidth() const;
        OsmAnd::VectorLineBuilder& setOutlineWidth(const double width);

        // Get/set outline color/filter (both near and far)
        FColorARGB getOutlineColor() const;
        OsmAnd::VectorLineBuilder& setOutlineColor(const FColorARGB color);

        // Get/set near outline color/filter (see get/set colorization mapping), which is:
        // for 2D vector line - color/filter for inner edge of outline,
        // for 3D vector line - color/filter for top edge of vertical trace wall
        FColorARGB getNearOutlineColor() const;
        OsmAnd::VectorLineBuilder& setNearOutlineColor(const FColorARGB color);

        // Get/set far outline color/filter (see get/set colorization mapping), which is:
        // for 2D vector line - color/filter for outer edge of outline,
        // for 3D vector line - color/filter for bottom edge of vertical trace wall
        FColorARGB getFarOutlineColor() const;
        OsmAnd::VectorLineBuilder& setFarOutlineColor(const FColorARGB color);

        // Get/set visibility of line with heights (3D only - see get/set heights):
        bool getElevatedLineVisibility() const;
        OsmAnd::VectorLineBuilder& setElevatedLineVisibility(const bool visible);

        // Get/set visibility of line on the surface (3D only - see get/set heights):
        bool getSurfaceLineVisibility() const;
        OsmAnd::VectorLineBuilder& setSurfaceLineVisibility(const bool visible);

        // Get/set scale factor for line elevation (affects distance from the surface - 3D only):
        float getElevationScaleFactor() const;
        OsmAnd::VectorLineBuilder& setElevationScaleFactor(const float scaleFactor);

        sk_sp<const SkImage> getPathIcon() const;
        VectorLineBuilder& setPathIcon(const SingleSkImage& image);

        sk_sp<const SkImage> getSpecialPathIcon() const;
        VectorLineBuilder& setSpecialPathIcon(const SingleSkImage& image);

        float getPathIconStep() const;
        VectorLineBuilder& setPathIconStep(const float step);

        float getSpecialPathIconStep() const;
        VectorLineBuilder& setSpecialPathIconStep(const float step);

        bool isPathIconOnSurface() const;
        VectorLineBuilder& setPathIconOnSurface(const bool onSurface);

        float getScreenScale() const;
        VectorLineBuilder& setScreenScale(const float step);

        // Get/set cap style for path ends (BUTT, SQUARE, ROUND, JOINT, ARROW)
        VectorLineBuilder& setEndCapStyle(const VectorLine::EndCapStyle endCapStyle);
        VectorLineBuilder& setEndCapStyle(const int endCapStyle);

        // Get/set style for path joints (MITER, BEVEL, ROUND)
        VectorLineBuilder& setJointStyle(const VectorLine::JointStyle jointStyle);
        VectorLineBuilder& setJointStyle(const int jointStyle);

        VectorLineBuilder& attachMarker(const std::shared_ptr<MapMarker>& marker);

        std::shared_ptr<OsmAnd::VectorLine> buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection);
        std::shared_ptr<OsmAnd::VectorLine> build();
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_BUILDER_H_)
