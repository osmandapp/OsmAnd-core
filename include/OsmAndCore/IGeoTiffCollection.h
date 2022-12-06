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
    private:
    protected:
        IGeoTiffCollection();
    public:
        virtual ~IGeoTiffCollection();

        virtual ZoomLevel getMaxZoom(const uint32_t tileSize) const = 0;

        virtual QString getGeoTiffFilename(const TileId& tileId, const ZoomLevel zoom,
            const uint32_t tileSize, const uint32_t overlap, const ZoomLevel minzoom = MinZoomLevel) const = 0;
        virtual bool getGeoTiffData(const QString& filename, const TileId& tileId, const ZoomLevel zoom,
            const uint32_t tileSize, const uint32_t overlap, void *pBuffer) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_GEOTIFF_COLLECTION_H_)
