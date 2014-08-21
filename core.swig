%module(directors="1") OsmAndCore

// Generate directors for all classes that have virtual methods
//%feature("director");

%{

// Common
#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

// Data-related
#include <OsmAndCore/Data/DataTypes.h>

// Map-related
#include <OsmAndCore/Map/MapTypes.h>

// To omit 'OsmAnd::' prefixes inside namespace itself, use that namespace
using namespace OsmAnd;

%}

%include <enums.swg>
%include <typemaps.i>
%include <stdint.i>
%include <std_shared_ptr.i>
%include <std_pair.i>
%include <std_string.i>
%include <swig/qstring.i>

//%shared_ptr(OsmAnd::MapStyle)

%apply QString { std::string };

// Common
%include "OsmAndCore/QuickAccessors.h"
%include "OsmAndCore/Callable.h"
%include "OsmAndCore/CommonTypes.h"

// Data-related
%include "OsmAndCore/Data/DataTypes.h"

// Map-related
%include "OsmAndCore/Map/MapTypes.h"
//%include "OsmAndCore/Map/MapRendererSetupOptions.h"
//%include "OsmAndCore/Map/MapRendererDebugSettings.h"
//%include "OsmAndCore/Map/MapRendererConfiguration.h"
//%include "OsmAndCore/Map/MapRendererState.h"
//%include "OsmAndCore/Map/MapRendererDebugSettings.h"
//%include "OsmAndCore/Map/IMapRenderer.h"

%template(PointI) OsmAnd::Point<int32_t>;
%template(PointI64) OsmAnd::Point<int64_t>;
%template(PointF) OsmAnd::Point<float>;
%template(PointD) OsmAnd::Point<double>;

%template(AreaI) OsmAnd::Area<int32_t>;
%template(AreaI64) OsmAnd::Area<int64_t>;
%template(AreaF) OsmAnd::Area<float>;
%template(AreaD) OsmAnd::Area<double>;

%template(OOBBI) OsmAnd::OOBB<int32_t>;
%template(OOBBI64) OsmAnd::OOBB<int64_t>;
%template(OOBBF) OsmAnd::OOBB<float>;
%template(OOBBD) OsmAnd::OOBB<double>;
