%module OsmAndCore

%include "typemaps.i"

%{

// Common
#include "OsmAndCore.h"
#include "OsmAndCore/Utilities.h"
#include "OsmAndCore/EmbeddedResources.h"
#include "OsmAndCore/CommonTypes.h"

// Map-related
#include "OsmAndCore/Map/IMapTileProvider.h"
#include "OsmAndCore/Map/IMapBitmapTileProvider.h"
#include "OsmAndCore/Map/IMapElevationDataProvider.h"
#include "OsmAndCore/Map/IMapRenderer.h"
#include "OsmAndCore/Map/HeightmapTileProvider.h"
#include "OsmAndCore/Map/HillshadeTileProvider.h"
#include "OsmAndCore/Map/OnlineMapRasterTileProvider.h"
#include "OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
#include "OsmAndCore/Routing/RoutingConfiguration.h"
#include "OsmAndCore/Routing/RoutingProfile.h"
#include "OsmAndCore/Routing/RoutePlanner.h"
#include "OsmAndCore/Routing/RoutePlannerContext.h"

%}

// Common
%include "OsmAndCore.h"
%include "OsmAndCore/Utilities.h"
%include "OsmAndCore/EmbeddedResources.h"
%include "OsmAndCore/CommonTypes.h"

// Map-related
%include "OsmAndCore/Map/IMapTileProvider.h"
%include "OsmAndCore/Map/IMapBitmapTileProvider.h"
%include "OsmAndCore/Map/IMapElevationDataProvider.h"
%include "OsmAndCore/Map/IMapRenderer.h"
%include "OsmAndCore/Map/HeightmapTileProvider.h"
%include "OsmAndCore/Map/HillshadeTileProvider.h"
%include "OsmAndCore/Map/OnlineMapRasterTileProvider.h"
%include "OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
%include "OsmAndCore/Routing/RoutingConfiguration.h"
%include "OsmAndCore/Routing/RoutingProfile.h"
%include "OsmAndCore/Routing/RoutePlanner.h"
%include "OsmAndCore/Routing/RoutePlannerContext.h"
