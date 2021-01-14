#ifndef _MULTIPOLYGONS_H
#define _MULTIPOLYGONS_H

#include <stdio.h>

#include <map>
#include <set>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "renderRules.h"

/// !!! Fuly copied from MapRenderRepositories.java, should be carefully synchroinized
bool isClockwiseWay(std::vector<int_pair>& c);

int ray_intersect_x(int prevX, int prevY, int x, int y, int middleY);

bool linesIntersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

bool calculateLineCoordinates(bool inside, int x, int y, bool pinside, int px, int py, int leftX, int rightX,
							  int bottomY, int topY, std::vector<int_pair>& coordinates);

void combineMultipolygonLine(std::vector<coordinates>& completedRings, std::vector<coordinates>& incompletedRings,
							 coordinates& coordinates);

void unifyIncompletedRings(std::vector<std::vector<int_pair> >& incompletedRings,
						   std::vector<std::vector<int_pair> >& completedRings, int leftX, int rightX, int bottomY,
						   int topY, int64_t dbId, int zoom);

bool processCoastlines(std::vector<FoundMapDataObject>& coastLines, int leftX, int rightX, int bottomY, int topY,
					   int zoom, bool showIfThereIncompleted, bool addDebugIncompleted,
					   std::vector<FoundMapDataObject>& res);

#endif
