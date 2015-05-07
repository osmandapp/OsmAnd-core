#include "AtlasMapRendererInternalState.h"

OsmAnd::AtlasMapRendererInternalState::AtlasMapRendererInternalState()
{
    visibleTiles.reserve(512);
    uniqueTiles.reserve(512);
}

OsmAnd::AtlasMapRendererInternalState::~AtlasMapRendererInternalState()
{
}
