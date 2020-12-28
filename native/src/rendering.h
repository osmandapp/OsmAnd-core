#ifndef _OSMAND_RENDERING_H
#define _OSMAND_RENDERING_H

#include <SkCanvas.h>

#include <vector>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "renderRules.h"

void doRendering(std::vector<FoundMapDataObject> &mapDataObjects, SkCanvas *canvas, RenderingRuleSearchRequest *req,
				 RenderingContext *rc);

#endif
