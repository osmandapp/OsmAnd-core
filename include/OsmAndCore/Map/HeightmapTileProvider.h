#ifndef _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>

namespace OsmAnd
{
    class HeightmapTileProvider_P;
    class OSMAND_CORE_API HeightmapTileProvider : public IMapElevationDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(HeightmapTileProvider);

    private:
        PrivateImplementation<HeightmapTileProvider_P> _p;
    protected:
    public:
        HeightmapTileProvider(const QString& dataPath, const QString& indexFilename = {});
        virtual ~HeightmapTileProvider();

        const QString dataPath;
        const QString indexFilename;

        void rebuildTileDbIndex();

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        virtual uint32_t getTileSize() const Q_DECL_OVERRIDE;

        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        static const QString defaultIndexFilename;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHTMAP_TILE_PROVIDER_H_)
