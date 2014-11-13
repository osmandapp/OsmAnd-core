#ifndef _OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API IMapTiledDataProvider : public IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapTiledDataProvider);
    public:
        class OSMAND_CORE_API Data : public IMapDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            TileId tileId;
            ZoomLevel zoom;
        };

    private:
    protected:
        IMapTiledDataProvider();
    public:
        virtual ~IMapTiledDataProvider();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom() const = 0;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_)
