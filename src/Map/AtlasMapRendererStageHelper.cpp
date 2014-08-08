#include "AtlasMapRendererStageHelper.h"

#include "AtlasMapRenderer.h"
#include "AtlasMapRendererStage.h"
#include "AtlasMapRendererInternalState.h"

OsmAnd::AtlasMapRendererStageHelper::AtlasMapRendererStageHelper(AtlasMapRendererStage* const stage_)
    : _stage(stage_)
{
}

OsmAnd::AtlasMapRendererStageHelper::~AtlasMapRendererStageHelper()
{
}

OsmAnd::AtlasMapRenderer* OsmAnd::AtlasMapRendererStageHelper::getRenderer() const
{
    return static_cast<AtlasMapRenderer*>(_stage->renderer);
}

const OsmAnd::AtlasMapRendererInternalState& OsmAnd::AtlasMapRendererStageHelper::getInternalState() const
{
    return static_cast<const OsmAnd::AtlasMapRendererInternalState&>(_stage->internalState);
}
