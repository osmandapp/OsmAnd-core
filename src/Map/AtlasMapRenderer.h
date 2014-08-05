#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRenderer.h"
#include "IAtlasMapRenderer.h"
#include "MapRendererResourcesManager.h"

namespace OsmAnd
{
    class AtlasMapRenderer
        : public MapRenderer
        , public IAtlasMapRenderer
    {
        Q_DISABLE_COPY(AtlasMapRenderer);

    private:
        // General:
        QSet<TileId> _uniqueTiles;
    protected:
        AtlasMapRenderer(GPUAPI* const gpuAPI);

        // State-related:
        virtual bool updateInternalState(MapRendererInternalState* internalState, const MapRendererState& state);

        // Customization points:
        virtual bool prePrepareFrame();
        virtual bool postPrepareFrame();
    public:
        virtual ~AtlasMapRenderer();

        virtual QList<TileId> getVisibleTiles() const;
        virtual unsigned int getVisibleTilesCount() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_H_)
