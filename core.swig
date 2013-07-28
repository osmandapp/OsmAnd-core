%module OsmAndCore

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
//#include "OsmAndCore/Map/HillshadeTileProvider.h"
#include "OsmAndCore/Map/OnlineMapRasterTileProvider.h"
#include "OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
#include "OsmAndCore/Routing/RoutingConfiguration.h"
#include "OsmAndCore/Routing/RoutingProfile.h"
#include "OsmAndCore/Routing/RoutePlanner.h"
#include "OsmAndCore/Routing/RoutePlannerContext.h"

// To omit 'OsmAnd::' prefixes inside namespace itself, use that namespace
using namespace OsmAnd;

%}

%include <typemaps.i>
%include <stdint.i>
%include <std_shared_ptr.i>
%shared_ptr(OsmAnd::Model::Road)
%shared_ptr(OsmAnd::RoutingConfiguration)
%shared_ptr(OsmAnd::RoutingProfileContext)
%shared_ptr(OsmAnd::RoutingRuleset)
%shared_ptr(OsmAnd::IMapTileProvider)
%shared_ptr(OsmAnd::IMapBitmapTileProvider)
%shared_ptr(OsmAnd::IMapElevationDataProvider)
%shared_ptr(OsmAnd::HeightmapTileProvider)
%shared_ptr(OsmAnd::OnlineMapRasterTileProvider)

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
//%include "OsmAndCore/Map/HillshadeTileProvider.h"
%include "OsmAndCore/Map/OnlineMapRasterTileProvider.h"
%include "OsmAndCore/Map/VectorMapTileProvider.h"

// Routing-related
%include "OsmAndCore/Routing/RoutingConfiguration.h"
%include "OsmAndCore/Routing/RoutingProfile.h"
%include "OsmAndCore/Routing/RoutePlanner.h"
%include "OsmAndCore/Routing/RoutePlannerContext.h"
