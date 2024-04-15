#ifndef _OSMAND_CORE_POLYGON_BUILDER_P_H_
#define _OSMAND_CORE_POLYGON_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QVector>
#include <QHash>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "Polygon.h"


namespace OsmAnd
{
    class PolygonsCollection;
    class PolygonsCollection_P;

    class PolygonBuilder;
    class PolygonBuilder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(PolygonBuilder_P);

    private:
    protected:
        PolygonBuilder_P(PolygonBuilder* const owner);

        mutable QReadWriteLock _lock;

        bool _isHidden;

        int _polygonId;
        int _baseOrder;

        FColorARGB _fillColor;

        QVector<PointI> _points;

        PointI _circleCenter;
        double _circleRadiusInMeters;

        float _direction;
 
    public:
        virtual ~PolygonBuilder_P();

        ImplementationInterface<PolygonBuilder> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        int getPolygonId() const;
        void setPolygonId(const int polygonId);

        int getBaseOrder() const;
        void setBaseOrder(const int baseOrder);

        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB baseColor);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI> poinst);

        void setCircle(const PointI& center, const double radiusInMeters);

        std::shared_ptr<Polygon> buildAndAddToCollection(const std::shared_ptr<PolygonsCollection>& collection);
        std::shared_ptr<Polygon> build();

    friend class OsmAnd::PolygonBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_POLYGON_BUILDER_P_H_)
