#ifndef _OSMAND_CORE_HEIGHT_RASTER_MAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_HEIGHT_RASTER_MAP_TILE_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QSet>
#include <QDir>
#include <QUrl>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMutex>
#include <QWaitCondition>

#include <OsmAndCore/IGeoTiffCollection.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IRasterMapLayerProvider.h"
#include "HeightRasterMapLayerProvider.h"
#include "IWebClient.h"

namespace OsmAnd
{
    class HeightRasterMapLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(HeightRasterMapLayerProvider_P);
    private:
    protected:
        HeightRasterMapLayerProvider_P(
            HeightRasterMapLayerProvider* owner,
            const QString& heightColorsFilename,
            const ZoomLevel minZoom,
            const ZoomLevel maxZoom,
            const uint32_t tileSize,
            const float densityFactor);
    public:
        virtual ~HeightRasterMapLayerProvider_P();

        ImplementationInterface<HeightRasterMapLayerProvider> owner;

        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
        const uint32_t tileSize;
        const float densityFactor;

        IGeoTiffCollection::ProcessingParameters procParameters;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::HeightRasterMapLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_HEIGHT_RASTER_MAP_TILE_PROVIDER_P_H_)
