#ifndef _OSMAND_CORE_I_GEOTIFF_COLLECTION_H_
#define _OSMAND_CORE_I_GEOTIFF_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IGeoTiffCollection
    {
        Q_DISABLE_COPY_AND_MOVE(IGeoTiffCollection);
    public:
        enum class CallResult
        {
            Empty,
            Completed,
            Failed
        };        
        enum class RasterType
        {
            Heightmap,
            Hillshade,
            Slope,
            Height
        };        
        struct ProcessingParameters
        {
            RasterType rasterType;
            QString colorsFilename;
            QString resultColorsFilename;
            QString intermediateColorsFilename;
        };
    private:
    protected:
        IGeoTiffCollection();
    public:
        virtual ~IGeoTiffCollection();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom(const uint32_t tileSize) const = 0;

        virtual CallResult getGeoTiffData(
            const TileId& tileId,
            const ZoomLevel zoom,
            const uint32_t tileSize,
            const uint32_t overlap,
            const uint32_t bandCount,
            const bool toBytes,
            float& minValue,
            float& maxValue,
            void* pBuffer,
            const ProcessingParameters* procParameters = nullptr) const = 0;

        // Example: calculateHeights(ZoomLevel14, MapRenderer::ElevationDataTileSize, points31, outHeights))
        virtual bool calculateHeights(
            const ZoomLevel zoom,
            const uint32_t tileSize,
            const QList<PointI>& points31,
            QList<float>& outHeights) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_GEOTIFF_COLLECTION_H_)
