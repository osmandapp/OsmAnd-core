%module OsmAndCore

%include "typemaps.i"

%{

// Common
#include "include/OsmAndCore.h"
#include "include/OsmAndCore/Utilities.h"
#include "include/OsmAndCore/EmbeddedResources.h"

// Map-related
#include "include/OsmAndCore/Map/IMapTileProvider.h"
#include "include/OsmAndCore/Map/IMapBitmapTileProvider.h"
#include "include/OsmAndCore/Map/IMapElevationDataProvider.h"
#include "include/OsmAndCore/Map/IMapRenderer.h"
#include "include/OsmAndCore/Map/HeightmapTileProvider.h"
#include "include/OsmAndCore/Map/HillshadeTileProvider.h"
#include "include/OsmAndCore/Map/OnlineMapRasterTileProvider.h"
#include "include/OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
#include "include/OsmAndCore/Routing/RoutingConfiguration.h"
#include "include/OsmAndCore/Routing/RoutingProfile.h"
#include "include/OsmAndCore/Routing/RoutePlanner.h"
#include "include/OsmAndCore/Routing/RoutePlannerContext.h"

%}

// Common
%include "include/OsmAndCore.h"
%include "include/OsmAndCore/Utilities.h"
%include "include/OsmAndCore/EmbeddedResources.h"

// Map-related
%include "include/OsmAndCore/Map/IMapTileProvider.h"
%include "include/OsmAndCore/Map/IMapBitmapTileProvider.h"
%include "include/OsmAndCore/Map/IMapElevationDataProvider.h"
%include "include/OsmAndCore/Map/IMapRenderer.h"
%include "include/OsmAndCore/Map/HeightmapTileProvider.h"
%include "include/OsmAndCore/Map/HillshadeTileProvider.h"
%include "include/OsmAndCore/Map/OnlineMapRasterTileProvider.h"
%include "include/OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
%include "include/OsmAndCore/Routing/RoutingConfiguration.h"
%include "include/OsmAndCore/Routing/RoutingProfile.h"
%include "include/OsmAndCore/Routing/RoutePlanner.h"
%include "include/OsmAndCore/Routing/RoutePlannerContext.h"
