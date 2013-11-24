#include "RasterizerContext_P.h"
#include "RasterizerContext.h"

#include <QWriteLocker>

#include "Rasterizer_P.h"
#include "RasterizerSharedContext.h"
#include "RasterizerSharedContext_P.h"
#include "MapObject.h"

#include <SkPathEffect.h>

OsmAnd::RasterizerContext_P::RasterizerContext_P( RasterizerContext* owner_ )
    : owner(owner_)
{
}

OsmAnd::RasterizerContext_P::~RasterizerContext_P()
{
    clear();
}

void OsmAnd::RasterizerContext_P::clear()
{
    // If context has binded shared context, remove all primitives groups
    // that are owned only current context
    if(owner->sharedContext)
    {
        for(auto itPrimitivesGroup = _primitivesGroups.cbegin(); itPrimitivesGroup != _primitivesGroups.cend(); ++itPrimitivesGroup)
        {
            const auto& group = *itPrimitivesGroup;

            // Skip removing group from shared context if it's being used somewhere else, except for
            // current context and shared context
            if(group.use_count() > 2)
                continue;

            auto& primitivesCacheLevel = owner->sharedContext->_d->_primitivesCacheLevels[_zoom];
            {
                QWriteLocker scopedLocker(&primitivesCacheLevel._primitivesCacheLock);

                primitivesCacheLevel._primitivesCache.remove(group->mapObject->id);
            }
        }
    }
    
    _primitivesGroups.clear();
    _polygons.clear();
    _polylines.clear();
    _points.clear();

    _symbols.clear();
}
