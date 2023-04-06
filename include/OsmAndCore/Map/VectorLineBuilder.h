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
        
        bool shouldShowArrows() const;
        VectorLineBuilder& setShouldShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        VectorLineBuilder& setApproximationEnabled(const bool enabled);
        
        int getLineId() const;
        VectorLineBuilder& setLineId(const int lineId);

        int getBaseOrder() const;
        VectorLineBuilder& setBaseOrder(const int baseOrder);

        double getLineWidth() const;
        VectorLineBuilder& setLineWidth(const double width);
        
        FColorARGB getFillColor() const;
        VectorLineBuilder& setFillColor(const FColorARGB fillColor);

        std::vector<double> getLineDash() const;
        VectorLineBuilder& setLineDash(const std::vector<double> dashPattern);

        QVector<PointI> getPoints() const;
        VectorLineBuilder& setPoints(const QVector<PointI>& points);
        
        QList<OsmAnd::FColorARGB> getColorizationMapping() const;
        OsmAnd::VectorLineBuilder& setColorizationMapping(const QList<OsmAnd::FColorARGB>& colorizationMapping);
        
        int getColorizationScheme() const;
        OsmAnd::VectorLineBuilder& setColorizationScheme(const int colorizationScheme);
        
        double getOutlineWidth() const;
        OsmAnd::VectorLineBuilder& setOutlineWidth(const double width);

        FColorARGB getOutlineColor() const;
        OsmAnd::VectorLineBuilder& setOutlineColor(const FColorARGB color);

        sk_sp<const SkImage> getPathIcon() const;
        VectorLineBuilder& setPathIcon(const sk_sp<const SkImage>& image);
        
        sk_sp<const SkImage> getSpecialPathIcon() const;
        VectorLineBuilder& setSpecialPathIcon(const sk_sp<const SkImage>& image);

        float getPathIconStep() const;
        VectorLineBuilder& setPathIconStep(const float step);
        
        float getSpecialPathIconStep() const;
        VectorLineBuilder& setSpecialPathIconStep(const float step);

        bool isPathIconOnSurface() const;
        VectorLineBuilder& setPathIconOnSurface(const bool onSurface);

        float getScreenScale() const;
        VectorLineBuilder& setScreenScale(const float step);

        VectorLineBuilder& setEndCapStyle(const VectorLine::EndCapStyle endCapStyle);
        VectorLineBuilder& setEndCapStyle(const int endCapStyle);

        std::shared_ptr<VectorLine> buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection);
        std::shared_ptr<VectorLine> build();
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_BUILDER_H_)
