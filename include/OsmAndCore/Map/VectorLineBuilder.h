#ifndef _OSMAND_CORE_VECTOR_LINE_BUILDER_H_
#define _OSMAND_CORE_VECTOR_LINE_BUILDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Map/VectorLine.h>

class SkBitmap;

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

        int getLineId() const;
        VectorLineBuilder& setLineId(const int lineId);

        int getBaseOrder() const;
        VectorLineBuilder& setBaseOrder(const int baseOrder);

        double getLineWidth() const;
        VectorLineBuilder& setLineWidth(const double width);
        FColorARGB getFillColor() const;
        VectorLineBuilder& setFillColor(const FColorARGB fillColor);

        QVector<PointI> getPoints() const;
        VectorLineBuilder& setPoints(const QVector<PointI>& points);

        std::shared_ptr<const SkBitmap> getPathIcon() const;
        VectorLineBuilder& setPathIcon(const std::shared_ptr<const SkBitmap>& bitmap);

        float getPathIconStep() const;
        VectorLineBuilder& setPathIconStep(const float step);

        std::shared_ptr<VectorLine> buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection);
        std::shared_ptr<VectorLine> build();
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_BUILDER_H_)
