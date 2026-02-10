#ifndef _OSMAND_CORE_POLYGON_H_
#define _OSMAND_CORE_POLYGON_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>

namespace OsmAnd
{
    class PolygonBuilder;
    class PolygonBuilder_P;
    class PolygonsCollection;
    class PolygonsCollection_P;

    class Polygon_P;
    typedef enum
    {
        kRoundJoinType = 0,
        kBezelJoinType,
        kMiterJoinType
        
    } kPolygonJoinType;
    
    class OSMAND_CORE_API Polygon
    {
        Q_DISABLE_COPY_AND_MOVE(Polygon);

    public:
        class SymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
            const std::shared_ptr<Polygon_P> _polygonP;
        protected:
            SymbolsGroup(const std::shared_ptr<Polygon_P>& polygonP);
        public:
            virtual ~SymbolsGroup();

            const Polygon* getPolygon() const;

            virtual bool updatesPresent();
            virtual UpdateResult update(const MapState& mapState);

            virtual bool supportsResourcesRenew();

        friend class OsmAnd::Polygon;
        friend class OsmAnd::Polygon_P;
        };

    private:
        PrivateImplementation<Polygon_P> _p;
    protected:
        Polygon(
            const int polygonId,
            const int baseOrder,
            const FColorARGB fillColor
        );

        bool applyChanges();
    public:
        virtual ~Polygon();

        const int polygonId;
        const int baseOrder;
        const FColorARGB fillColor;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);

        bool hasUnappliedChanges() const;

        std::shared_ptr<SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::PolygonBuilder;
    friend class OsmAnd::PolygonBuilder_P;
    friend class OsmAnd::PolygonsCollection;
    friend class OsmAnd::PolygonsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_POLYGON_H_)
