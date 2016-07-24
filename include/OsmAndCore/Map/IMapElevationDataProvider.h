#ifndef _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapElevationDataProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapElevationDataProvider)
    public:
        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data)
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const size_t rowLength,
                const uint32_t size,
                const float* const pRawData);
            virtual ~Data();

            const size_t rowLength;
            const uint32_t size;
            const float* pRawData;
        };

    private:
    protected:
        IMapElevationDataProvider();
    public:
        virtual ~IMapElevationDataProvider();

        virtual unsigned int getTileSize() const = 0;

        virtual bool obtainElevationData(
            const Request& request,
            std::shared_ptr<Data>& outElevationData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_)
