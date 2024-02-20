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

        struct OSMAND_CORE_API Request : public IMapDataProvider::Request
        {
            Request();
            Request(const IMapDataProvider::Request& that);
            virtual ~Request();

            TileId tileId;
            ZoomLevel zoom;
            bool cacheOnly;

            static void copy(Request& dst, const IMapDataProvider::Request& src);
            virtual std::shared_ptr<IMapDataProvider::Request> clone() const Q_DECL_OVERRIDE;

        private:
            typedef IMapDataProvider::Request super;
        protected:
            Request(const Request& that);
        };

    private:
    protected:
        IMapTiledDataProvider();
    public:
        virtual ~IMapTiledDataProvider();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom() const = 0;

        virtual ZoomLevel getMinVisibleZoom() const;
        virtual ZoomLevel getMaxVisibleZoom() const;
        
        virtual int getMaxMissingDataZoomShift() const;
        virtual int getMaxMissingDataUnderZoomShift() const;

        virtual bool isMetaTiled() const;

        virtual bool obtainTiledData(
            const Request& request,
            std::shared_ptr<Data>& outTiledData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_TILED_DATA_PROVIDER_H_)
