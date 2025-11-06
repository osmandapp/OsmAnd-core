#ifndef _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_
#define _OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QVector>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapTiledDataProvider.h"
#include "MapPrimitivesProvider.h"
#include "Map3DObjectsProvider.h"

namespace OsmAnd
{
    class Map3DObjectsTiledProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Map3DObjectsTiledProvider_P);

    private:
        std::shared_ptr<MapPrimitivesProvider> _tiledProvider;

    protected:
        Map3DObjectsTiledProvider_P(Map3DObjectsTiledProvider* const owner,
            const std::shared_ptr<MapPrimitivesProvider>& tiledProvider);

    public:
        ~Map3DObjectsTiledProvider_P();

        ImplementationInterface<Map3DObjectsTiledProvider> owner;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

        bool obtainTiledData(
            const IMapTiledDataProvider::Request& request,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* const pOutMetric);

        bool supportsNaturalObtainData() const;
        bool supportsNaturalObtainDataAsync() const;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        friend class OsmAnd::Map3DObjectsTiledProvider;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_3D_OBJECTS_PROVIDER_P_H_)


