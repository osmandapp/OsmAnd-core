#ifndef _OSMAND_CORE_I_MAP_LAYER_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapLayerProvider);
    private:
    protected:
        IMapLayerProvider();
    public:
        virtual ~IMapLayerProvider();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_LAYER_PROVIDER_H_)
