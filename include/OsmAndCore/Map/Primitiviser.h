#ifndef _OSMAND_CORE_PRIMITIVISER_H_
#define _OSMAND_CORE_PRIMITIVISER_H_

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
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapStyleEvaluationResult.h>
#include <OsmAndCore/Map/Primitiviser_Metrics.h>

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }
    class MapPresentationEnvironment;

    class Primitiviser_P;
    class OSMAND_CORE_API Primitiviser
    {
        Q_DISABLE_COPY_AND_MOVE(Primitiviser);
    public:
        enum class PrimitiveType : uint32_t
        {
            Point = 1,
            Polyline = 2,
            Polygon = 3,
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
            PrimitivesGroup(const std::shared_ptr<const Model::BinaryMapObject>& sourceObject);
        public:
            ~PrimitivesGroup();

            const std::shared_ptr<const Model::BinaryMapObject> sourceObject;

            PrimitivesCollection polygons;
            PrimitivesCollection polylines;
            PrimitivesCollection points;

        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
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
                const MapStyleEvaluationResult& evaluationResult);

#ifdef Q_COMPILER_RVALUE_REFS
            Primitive(
                const std::shared_ptr<const PrimitivesGroup>& group,
                const PrimitiveType type,
                const uint32_t typeRuleIdIndex,
                MapStyleEvaluationResult&& evaluationResult);
#endif // Q_COMPILER_RVALUE_REFS
        public:
            ~Primitive();

            const std::weak_ptr<const PrimitivesGroup> group;
            const std::shared_ptr<const Model::BinaryMapObject> sourceObject;

            const PrimitiveType type;
            const uint32_t typeRuleIdIndex;
            const MapStyleEvaluationResult evaluationResult;

            double zOrder;

        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
        };

        class Symbol;
        typedef QList< std::shared_ptr<const Symbol> > SymbolsCollection;

        class SymbolsGroup;
        typedef QHash< std::shared_ptr<const Model::BinaryMapObject>, std::shared_ptr<const SymbolsGroup> > SymbolsGroupsCollection;

        class OSMAND_CORE_API SymbolsGroup Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(SymbolsGroup);
        private:
        protected:
            SymbolsGroup(const std::shared_ptr<const Model::BinaryMapObject>& sourceObject);
        public:
            ~SymbolsGroup();

            const std::shared_ptr<const Model::BinaryMapObject> sourceObject;

            SymbolsCollection symbols;
        
        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
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

        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
        };

        class OSMAND_CORE_API TextSymbol : public Symbol
        {
            Q_DISABLE_COPY_AND_MOVE(TextSymbol);
        private:
        protected:
            TextSymbol(const std::shared_ptr<const Primitive>& primitive);
        public:
            virtual ~TextSymbol();

            QString value;
            LanguageId languageId;
            bool drawOnPath;
            int verticalOffset;
            ColorARGB color;
            int size;
            int shadowRadius;
            ColorARGB shadowColor;
            int wrapWidth;
            bool isBold;
            PointI minDistance;
            QString shieldResourceName;

        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
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
            QString shieldResourceName;
            float intersectionSize;

        friend class OsmAnd::Primitiviser;
        friend class OsmAnd::Primitiviser_P;
        };

        class OSMAND_CORE_API Cache
        {
            Q_DISABLE_COPY_AND_MOVE(Cache);
        public:
            typedef SharedResourcesContainer<uint64_t, const PrimitivesGroup> SharedPrimitivesGroupsContainer;
            typedef SharedResourcesContainer<uint64_t, const SymbolsGroup> SharedSymbolsGroupsContainer;

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
        
        class OSMAND_CORE_API PrimitivisedArea Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(PrimitivisedArea);
        private:
            const std::weak_ptr<Cache> _cache;
        protected:
            PrimitivisedArea(
                const AreaI area31,
                const PointI sizeInPixels,
                const ZoomLevel zoom,
                const std::shared_ptr<Cache>& cache,
                const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment);
        public:
            ~PrimitivisedArea();

            const AreaI area31;
            const PointI sizeInPixels;
            const ZoomLevel zoom;

            const std::shared_ptr<const MapPresentationEnvironment> mapPresentationEnvironment;

            ColorARGB defaultBackgroundColor;
            int shadowRenderingMode;
            ColorARGB shadowRenderingColor;
            double polygonAreaMinimalThreshold;
            unsigned int roadDensityZoomTile;
            unsigned int roadsDensityLimitPerTile;
            int shadowLevelMin;
            int shadowLevelMax;
            PointD scale31ToPixelDivisor;

            PrimitivesGroupsCollection primitivesGroups;
            PrimitivesCollection polygons;
            PrimitivesCollection polylines;
            PrimitivesCollection points;

            SymbolsGroupsCollection symbolsGroups;

            bool isEmpty() const;

            friend class OsmAnd::Primitiviser;
            friend class OsmAnd::Primitiviser_P;
        };
    private:
        PrivateImplementation<Primitiviser_P> _p;
    protected:
    public:
        Primitiviser(const std::shared_ptr<const MapPresentationEnvironment>& environment);
        virtual ~Primitiviser();

        const std::shared_ptr<const MapPresentationEnvironment> environment;
        
        std::shared_ptr<const PrimitivisedArea> primitivise(
            const AreaI area31,
            const PointI sizeInPixels,
            const ZoomLevel zoom,
            const MapFoundationType foundation,
            const QList< std::shared_ptr<const Model::BinaryMapObject> >& objects,
            const std::shared_ptr<Cache>& cache = nullptr,
            const IQueryController* const controller = nullptr,
            Primitiviser_Metrics::Metric_primitivise* const metric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_PRIMITIVISER_H_)
