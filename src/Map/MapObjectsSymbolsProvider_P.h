#ifndef _OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledSymbolsProvider.h"
#include "MapObject.h"
#include "MapObjectsSymbolsProvider.h"

namespace OsmAnd
{
    class MapSymbolsGroup;

    class MapObjectsSymbolsProvider_P Q_DECL_FINAL
    {
    public:
        typedef MapObjectsSymbolsProvider::FilterCallback FilterCallback;
        typedef MapObjectsSymbolsProvider::MapObjectSymbolsGroup MapObjectSymbolsGroup;

    private:
        typedef MapObjectsSymbolsProvider::Request Request;
        typedef OsmAnd::MapPrimitiviser::PrimitivisedObjects PrimitivisedObjects;
        typedef OsmAnd::MapPrimitiviser::Symbol PrimitivisedSymbol;

        class СombinedPath
        {
        private:
            QVector<PointI> _points;
            mutable float _lengthInPixels;
            bool _attachedToAnotherPath;
            bool _combined;
        public:
            СombinedPath(
                const std::shared_ptr<const MapObject>& mapObject,
                const PointD& divisor31ToPixels);
            virtual ~СombinedPath();

            const std::shared_ptr<const MapObject> mapObject;
            const PointD divisor31ToPixels;

            QVector<PointI> getPoints() const;
            bool isCombined() const;

            bool isAttachedToAnotherPath() const;
            bool isAttachAllowed(
                const std::shared_ptr<const СombinedPath>& other,
                const float maxGapBetweenPaths,
                float& outGapBetweenPaths) const;
            void attachPath(const std::shared_ptr<СombinedPath>& other);

            float getLengthInPixels() const;
        };

        struct CombinePathsResult
        {
            QHash< std::shared_ptr<const MapObject>, QVector<PointI> > combinedPaths;
            QList< std::shared_ptr<const MapObject> > mapObjectToSkip;

            QVector<PointI> getCombinedOrOriginalPath(const std::shared_ptr<const MapObject>& mapObject) const
            {
                const auto& citCombinedPath = combinedPaths.constFind(mapObject);
                if (citCombinedPath == combinedPaths.cend())
                    return mapObject->points31;
                else
                    return *citCombinedPath;
            }
        };

        struct ComputedPinPoint
        {
            PointI point31;

            unsigned int basePathPointIndex;
            float normalizedOffsetFromBasePathPoint;
        };

        CombinePathsResult combineOnPathSymbols(
            const MapPrimitiviser::SymbolsGroupsCollection& symbolsGroups,
            const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment,
            const PointD& scaleDivisor31ToPixel,
            const std::shared_ptr<const IQueryController>& queryController) const;

        QList<ComputedPinPoint> computePinPoints(
            const QVector<PointI>& path31,
            const float globalPaddingInPixels,
            const float blockSpacingInPixels,
            const float symbolSpacingInPixels,
            const QVector<float>& symbolsWidthsInPixels,
            const ZoomLevel neededZoom) const;

        void computeSymbolsPinPoints(
            const QVector<float>& symbolsWidthsN,
            const int symbolsCount,
            float nextSymbolStartN,
            const QVector<float>& pathSegmentsLengthN,
            const QVector<PointI>& path31,
            const float symbolSpacingN,
            QList<ComputedPinPoint>& outComputedPinPoints) const;

        static bool hasOnPathSymbol(const std::shared_ptr<const MapPrimitiviser::SymbolsGroup>& symbolsGroup)
        {
            return std::any_of(symbolsGroup->symbols,
                []
                (const std::shared_ptr<const PrimitivisedSymbol>& symbol) -> bool
                {
                    if (const auto textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol))
                        return textSymbol->drawOnPath;

                    return false;
                });
        }

    protected:
        MapObjectsSymbolsProvider_P(MapObjectsSymbolsProvider* owner);

        ImplementationInterface<MapObjectsSymbolsProvider> owner;

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata(
                const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata);
            virtual ~RetainableCacheMetadata();

            std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata> binaryMapPrimitivesRetainableCacheMetadata;
        };
    public:
        virtual ~MapObjectsSymbolsProvider_P();

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::MapObjectsSymbolsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_OBJECTS_SYMBOLS_PROVIDER_P_H_)
