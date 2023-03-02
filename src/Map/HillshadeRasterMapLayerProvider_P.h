#ifndef _OSMAND_CORE_HILLSHADE_RASTER_MAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_HILLSHADE_RASTER_MAP_TILE_PROVIDER_P_H_

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
#include "HillshadeRasterMapLayerProvider.h"
#include "IWebClient.h"

namespace OsmAnd
{
    class HillshadeRasterMapLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(HillshadeRasterMapLayerProvider_P);
    private:
    protected:
        HillshadeRasterMapLayerProvider_P(
            HillshadeRasterMapLayerProvider* owner,
            const QString& hillshadeColorsFilename,
            const QString& slopeColorsFilename,
            const uint32_t tileSize,
            const float densityFactor);
    public:
        virtual ~HillshadeRasterMapLayerProvider_P();

        ImplementationInterface<HillshadeRasterMapLayerProvider> owner;

        const uint32_t tileSize;
        const float densityFactor;

        IGeoTiffCollection::ProcessingParameters procParameters;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::HillshadeRasterMapLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_HILLSHADE_RASTER_MAP_TILE_PROVIDER_P_H_)
