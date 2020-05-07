#ifndef _OSMAND_RENDERING_H
#define _OSMAND_RENDERING_H

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "renderRules.h"
#include <vector>
#include <SkCanvas.h>

void doRendering(shared_ptr<std::vector<MapDataObject *>>  mapDataObjects, SkCanvas *canvas,
				 RenderingRuleSearchRequest *req, RenderingContext *rc);

#endif
