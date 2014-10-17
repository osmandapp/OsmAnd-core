#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/BinaryMapDataProvider_Metrics.h>
#include <OsmAndCore/Data/ObfMapSectionReader.h>

namespace OsmAnd
{
    class IObfsCollection;
    namespace Model
    {
        class BinaryMapObject;
    }

    class BinaryMapDataProvider_P;
    class OSMAND_CORE_API BinaryMapDataProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapDataProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const MapFoundationType tileFoundation,
                const QList< std::shared_ptr<const Model::BinaryMapObject> >& mapObjects,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            MapFoundationType tileFoundation;
            QList< std::shared_ptr<const Model::BinaryMapObject> > mapObjects;
        };

    private:
        PrivateImplementation<BinaryMapDataProvider_P> _p;
    protected:
    public:
        BinaryMapDataProvider(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~BinaryMapDataProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            BinaryMapDataProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_)
