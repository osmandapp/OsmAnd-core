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
        Q_DISABLE_COPY_AND_MOVE(IMapElevationDataProvider);
    public:
        class OSMAND_CORE_API Data : public IMapTiledDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
            inline float getInterpolatedValue(const float x, const float y) const;
        protected:
        public:
            Data(TileId tileId, ZoomLevel zoom, size_t rowLength, uint32_t size, const float* pRawData);
            ~Data() override;

            const size_t rowLength;
            const uint32_t size;
            const float* pRawData;
            const float heixelSizeN;
            const float halfHeixelSizeN;

            bool getValue(const PointF& coordinates, float& outValue) const;
            bool getClosestPoint(const float startElevationFactor, const float endElevationFactor,
                const PointF& startCoordinates, const float startElevation,
                const PointF& endCoordinates, const float endElevation,
                PointF& outCoordinates, float* outHeightInMeters = nullptr) const;
        };

    private:
    protected:
        IMapElevationDataProvider();
    public:
        ~IMapElevationDataProvider() override;

        SWIG_OMIT(Q_REQUIRED_RESULT) virtual unsigned int getTileSize() const = 0;

        virtual bool obtainElevationData(const Request& request, std::shared_ptr<Data>& outElevationData, std::shared_ptr<Metric>* pOutMetric);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_ELEVATION_DATA_PROVIDER_H_)
