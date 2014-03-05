%module OsmAndCore

%{

// Common
#include <OsmAndCore.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/EmbeddedResources.h>
#include <OsmAndCore/CommonTypes.h>

// Data-related
#include <OsmAndCore/Data/ObfsCollection.h>

// Map-related
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/MapStyles.h>
#include <OsmAndCore/Map/MapStyle.h>


// To omit 'OsmAnd::' prefixes inside namespace itself, use that namespace
using namespace OsmAnd;

%}

%include <typemaps.i>
%include <arrays_java.i>
%include <various.i>
%include <stdint.i>
%include <std_shared_ptr.i>
%include <std_pair.i>
%include <std_string.i>
%include <swig/qstring.i>

%shared_ptr(OsmAnd::MapStyle)
%shared_ptr(OsmAnd::IMapProvider)
%shared_ptr(OsmAnd::IMapTileProvider)
%shared_ptr(OsmAnd::IMapBitmapTileProvider)
%shared_ptr(OsmAnd::IMapElevationDataProvider)
%shared_ptr(OsmAnd::IMapSymbolProvider)

%apply QString { std::string };

// Common
%include "OsmAndCore.h"
%include "OsmAndCore/Utilities.h"
%include "OsmAndCore/EmbeddedResources.h"
%include "OsmAndCore/CommonTypes.h"

// Data-related
%include "OsmAndCore/Data/ObfsCollection.h"

// Map-related
%include "OsmAndCore/Map/MapTypes.h"
%include "OsmAndCore/Map/MapRendererTypes.h"
%include "OsmAndCore/Map/MapRendererState.h"
%include "OsmAndCore/Map/MapRendererConfiguration.h"
%include "OsmAndCore/Map/MapRendererSetupOptions.h"
%include "OsmAndCore/Map/IMapRenderer.h"
%include "OsmAndCore/Map/MapStyles.h"
%include "OsmAndCore/Map/MapStyle.h"
%include "OsmAndCore/Map/IRetainableResource.h"
%include "OsmAndCore/Map/IMapProvider.h"
%include "OsmAndCore/Map/IMapTileProvider.h"
%include "OsmAndCore/Map/IMapBitmapTileProvider.h"
%include "OsmAndCore/Map/IMapElevationDataProvider.h"
%include "OsmAndCore/Map/IMapSymbolProvider.h"

%template(PointI) OsmAnd::Point<int32_t>;
%template(PointI64) OsmAnd::Point<int64_t>;
%template(PointF) OsmAnd::Point<float>;
%template(PointD) OsmAnd::Point<double>;

//NOTE: Seems if template class has methods, it doesn't need to be instanced like this
//%template(AreaI) OsmAnd::Area<int32_t>;
//%template(AreaI64) OsmAnd::Area<int64_t>;
//%template(AreaF) OsmAnd::Area<float>;
//%template(AreaD) OsmAnd::Area<double>;
