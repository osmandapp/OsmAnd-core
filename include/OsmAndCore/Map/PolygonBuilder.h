#ifndef _OSMAND_CORE_POLYGON_BUILDER_H_
#define _OSMAND_CORE_POLYGON_BUILDER_H_

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
#include <OsmAndCore/Map/Polygon.h>

namespace OsmAnd
{
    class PolygonsCollection;

    class PolygonBuilder_P;
    class OSMAND_CORE_API PolygonBuilder
    {
        Q_DISABLE_COPY_AND_MOVE(PolygonBuilder);

    private:
        PrivateImplementation<PolygonBuilder_P> _p;
    protected:
    public:
        PolygonBuilder();
        virtual ~PolygonBuilder();

        bool isHidden() const;
        PolygonBuilder& setIsHidden(const bool hidden);

        int getPolygonId() const;
        PolygonBuilder& setPolygonId(const int polygonId);

        int getBaseOrder() const;
        PolygonBuilder& setBaseOrder(const int baseOrder);

        FColorARGB getFillColor() const;
        PolygonBuilder& setFillColor(const FColorARGB fillColor);

        QVector<PointI> getPoints() const;
        PolygonBuilder& setPoints(const QVector<PointI>& points);

        std::shared_ptr<Polygon> buildAndAddToCollection(const std::shared_ptr<PolygonsCollection>& collection);
        std::shared_ptr<Polygon> build();
    };
}

#endif // !defined(_OSMAND_CORE_POLYGON_BUILDER_H_)
