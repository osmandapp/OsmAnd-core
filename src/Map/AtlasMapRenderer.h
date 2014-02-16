#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRenderer.h"
#include "MapRendererResources.h"

namespace OsmAnd
{
    class AtlasMapRenderer : public MapRenderer
    {
        Q_DISABLE_COPY(AtlasMapRenderer);
    public:
        enum {
            TileSize3D = 100u,
            DefaultReferenceTileSizeOnScreen = 256u,
        };
    private:
    protected:
        AtlasMapRenderer(GPUAPI* const gpuAPI);
    public:
        virtual ~AtlasMapRenderer();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_H_)
