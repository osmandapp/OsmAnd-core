#ifndef _OSMAND_CORE_MAP_PRIMITIVISER_H_
#define _OSMAND_CORE_MAP_PRIMITIVISER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/SharedResourcesContainer.h>
#include <OsmAndCore/Data/MapObject.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapStyleEvaluationResult.h>
#include <OsmAndCore/Map/MapPrimitiviser_Metrics.h>

namespace OsmAnd
{
    class MapObject;
    class MapPresentationEnvironment;

    class MapPrimitiviser_P;
    class OSMAND_CORE_API MapPrimitiviser
    {
        Q_DISABLE_COPY_AND_MOVE(MapPrimitiviser);
    public:
        enum class PrimitiveType : uint32_t
        {
            Point = 1,
            Polyline = 2,
            Polygon = 3,
        };

        enum {
            LastZoomToUseBasemap = ZoomLevel11,
            DetailedLandDataMinZoom = ZoomLevel12,
            DefaultTextLabelWrappingLengthInCharacters = 20
        };

        class OSMAND_CORE_API CoastlineMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(CoastlineMapObject);

        private:
        protected:
        public:
            CoastlineMapObject();
            virtual ~CoastlineMapObject();
        };

        class OSMAND_CORE_API SurfaceMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(SurfaceMapObject);

        private:
        protected:
        public:
            SurfaceMapObject();
            virtual ~SurfaceMapObject();
        };

        class Primitive;
        typedef QList< std::shared_ptr<const Primitive> > PrimitivesCollection;

        class PrimitivesGroup;
        typedef QList< std::shared_ptr<const PrimitivesGroup> > PrimitivesGroupsCollection;

        class OSMAND_CORE_API PrimitivesGroup Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(PrimitivesGroup);
        private:
        protected:
            PrimitivesGroup(const std::shared_ptr<const MapObject>& sourceObject);
        public:
            ~PrimitivesGroup();

            const std::shared_ptr<const MapObject> sourceObject;

            PrimitivesCollection polygons;
            PrimitivesCollection polylines;
            PrimitivesCollection points;

        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };
        
        class OSMAND_CORE_API Primitive Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(Primitive);
        private:
        protected:
            Primitive(
                const std::shared_ptr<const PrimitivesGroup>& group,
                const PrimitiveType type,
                const uint32_t typeRuleIdIndex);

            Primitive(
                const std::shared_ptr<const PrimitivesGroup>& group,
                const PrimitiveType type,
                const uint32_t typeRuleIdIndex,
                const MapStyleEvaluationResult& evaluationResult,
                const float detailScaleFactor = 1.0f);
        public:
            ~Primitive();

            const std::weak_ptr<const PrimitivesGroup> group;
            const std::shared_ptr<const MapObject> sourceObject;

            const PrimitiveType type;
            const uint32_t attributeIdIndex;
            const MapStyleEvaluationResult::Packed evaluationResult;

            int zOrder;
            int64_t doubledArea;
            float detailScaleFactor;

        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class Symbol;
        typedef QList< std::shared_ptr<const Symbol> > SymbolsCollection;

        class SymbolsGroup;
        typedef QHash< std::shared_ptr<const MapObject>, std::shared_ptr<const SymbolsGroup> > SymbolsGroupsCollection;

        class OSMAND_CORE_API SymbolsGroup Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(SymbolsGroup);
        private:
        protected:
            SymbolsGroup(
                const std::shared_ptr<const MapObject>& sourceObject, bool canBeShownWithoutIcon = false);
        public:
            ~SymbolsGroup();

            const std::shared_ptr<const MapObject> sourceObject;
            bool canBeShownWithoutIcon;

            SymbolsCollection symbols;
        
        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class OSMAND_CORE_API GridSymbolsGroup Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(GridSymbolsGroup);
        private:
        protected:
            GridSymbolsGroup(
                const std::shared_ptr<const SymbolsGroup>& symbolsGroup,
                const bool canBeShared,
                const MapObject::SharingKey sharingKey);
        public:
            ~GridSymbolsGroup();

            const std::shared_ptr<const SymbolsGroup> symbolsGroup;
            const bool canBeShared;
            const MapObject::SharingKey sharingKey;
        
        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class OSMAND_CORE_API Symbol
        {
            Q_DISABLE_COPY_AND_MOVE(Symbol);
        private:
        protected:
            Symbol(const std::shared_ptr<const Primitive>& primitive);
        public:
            virtual ~Symbol();

            const std::shared_ptr<const Primitive> primitive;

            PointI location31;
            int order;
            bool drawAlongPath;
            QSet<QString> intersectsWith;
            float intersectionSizeFactor;
            float intersectionSize;
            float intersectionMargin;
            float minDistance;
            float scaleFactor;
            bool ignoreClick;

            bool operator==(const Symbol& that) const;
            bool operator!=(const Symbol& that) const;

            bool hasSameContentAs(const Symbol& that) const;
            bool hasDifferentContentAs(const Symbol& that) const;

        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class OSMAND_CORE_API TextSymbol : public Symbol
        {
            Q_DISABLE_COPY_AND_MOVE(TextSymbol);
        public:
            enum Placement
            {
                Top,
                Default,
                BottomAdditional
            };
        private:
        protected:
            TextSymbol(const std::shared_ptr<const Primitive>& primitive);
        public:
            virtual ~TextSymbol();

            QString baseValue;
            QString value;
            LanguageId languageId;
            bool drawOnPath;
            Placement placement;
            QList<Placement> additionalPlacements;
            int verticalOffset;
            ColorARGB color;
            int size;
            int shadowRadius;
            ColorARGB shadowColor;
            int wrapWidth;
            bool isBold;
            bool isItalic;
            QString shieldResourceName;
            QList<QString> underlayIconResourceNames;

            bool operator==(const TextSymbol& that) const;
            bool operator!=(const TextSymbol& that) const;

            bool hasSameContentAs(const TextSymbol& that) const;
            bool hasDifferentContentAs(const TextSymbol& that) const;

            static Placement placementFromString(const QString& placement);
            
        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class OSMAND_CORE_API IconSymbol : public Symbol
        {
            Q_DISABLE_COPY_AND_MOVE(IconSymbol);
        private:
        protected:
            IconSymbol(const std::shared_ptr<const Primitive>& primitive);
        public:
            virtual ~IconSymbol();

            QString resourceName;
            QList<QString> underlayResourceNames;
            QList<QString> overlayResourceNames;
            PointF offsetFactor;
            QString shieldResourceName;

            bool operator==(const IconSymbol& that) const;
            bool operator!=(const IconSymbol& that) const;

            bool hasSameContentAs(const IconSymbol& that) const;
            bool hasDifferentContentAs(const IconSymbol& that) const;

        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };

        class OSMAND_CORE_API Cache
        {
            Q_DISABLE_COPY_AND_MOVE(Cache);
        public:
            typedef SharedResourcesContainer<MapObject::SharingKey, const PrimitivesGroup> SharedPrimitivesGroupsContainer;
            typedef SharedResourcesContainer<MapObject::SharingKey, const SymbolsGroup> SharedSymbolsGroupsContainer;

        private:
        protected:
            std::array<SharedPrimitivesGroupsContainer, ZoomLevelsCount> _sharedPrimitivesGroups;
            std::array<SharedSymbolsGroupsContainer, ZoomLevelsCount> _sharedSymbolsGroups;
        public:
            Cache();
            virtual ~Cache();

            virtual SharedPrimitivesGroupsContainer& getPrimitivesGroups(const ZoomLevel zoom);
            virtual const SharedPrimitivesGroupsContainer& getPrimitivesGroups(const ZoomLevel zoom) const;
            virtual SharedSymbolsGroupsContainer& getSymbolsGroups(const ZoomLevel zoom);
            virtual const SharedSymbolsGroupsContainer& getSymbolsGroups(const ZoomLevel zoom) const;
            
            SharedPrimitivesGroupsContainer* getPrimitivesGroupsPtr(const ZoomLevel zoom);
            const SharedPrimitivesGroupsContainer* getPrimitivesGroupsPtr(const ZoomLevel zoom) const;
            SharedSymbolsGroupsContainer* getSymbolsGroupsPtr(const ZoomLevel zoom);
            const SharedSymbolsGroupsContainer* getSymbolsGroupsPtr(const ZoomLevel zoom) const;
        };
        
        class OSMAND_CORE_API PrimitivisedObjects Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(PrimitivisedObjects);
        private:
            const std::weak_ptr<Cache> _cache;
        protected:
            PrimitivisedObjects(
                const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment,
                const std::shared_ptr<Cache>& cache,
                const ZoomLevel zoom,
                const PointD scaleDivisor31ToPixel);
        public:
            ~PrimitivisedObjects();

            const std::shared_ptr<const MapPresentationEnvironment> mapPresentationEnvironment;
            const ZoomLevel zoom;
            const PointD scaleDivisor31ToPixel;

            PrimitivesGroupsCollection primitivesGroups;
            PrimitivesCollection polygons;
            PrimitivesCollection polylines;
            PrimitivesCollection points;

            SymbolsGroupsCollection symbolsGroups;

            bool isEmpty() const;

        friend class OsmAnd::MapPrimitiviser;
        friend class OsmAnd::MapPrimitiviser_P;
        };
    private:
        PrivateImplementation<MapPrimitiviser_P> _p;
    protected:
    public:
        MapPrimitiviser(const std::shared_ptr<const MapPresentationEnvironment>& environment);
        virtual ~MapPrimitiviser();

        const std::shared_ptr<const MapPresentationEnvironment> environment;

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const ZoomLevel zoom,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric = nullptr);

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric = nullptr);

        std::shared_ptr<PrimitivisedObjects> primitiviseAllMapObjects(
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric = nullptr);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithSurface(
            const AreaI area31,
            const PointI areaSizeInPixels,
            const ZoomLevel zoom,
            const ZoomLevel detailedZoom,
            const TileId tileId,
            const AreaI visibleArea31,
            const int64_t visibleAreaTime,
            const MapSurfaceType surfaceType,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric = nullptr);

        std::shared_ptr<PrimitivisedObjects> primitiviseWithoutSurface(
            const PointD scaleDivisor31ToPixel,
            const ZoomLevel zoom,
            const TileId tileId,
            const QList< std::shared_ptr<const MapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr,
            MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVISER_H_)
