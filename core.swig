%module(directors="1") OsmAndCore
%nspace;

%{
#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/ResourcesManager.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/IMapStylesPresetsCollection.h>
#include <OsmAndCore/Map/MapStylePreset.h>

// To omit 'OsmAnd::' prefixes inside namespace itself in headers, use that namespace for SWIG
using namespace OsmAnd;
%}

%include <enums.swg>
%include <typemaps.i>
%include <stdint.i>
%include <std_shared_ptr.i>
%include <std_pair.i>
%include <std_string.i>
%include <swig/qstring.i>

%apply QString { std::string };

%shared_ptr(OsmAnd::IObfsCollection)
%shared_ptr(OsmAnd::ObfDataInterface)
%shared_ptr(OsmAnd::ResourcesManager::Resource)
%shared_ptr(OsmAnd::ResourcesManager::BuiltinResource)
%shared_ptr(OsmAnd::ResourcesManager::LocalResource)
%shared_ptr(OsmAnd::ResourcesManager::ResourceInRepository)
%shared_ptr(OsmAnd::ResourcesManager::InstalledResource)
%shared_ptr(OsmAnd::ResourcesManager::UnmanagedResource)
%shared_ptr(OsmAnd::IMapStylesCollection)
%shared_ptr(OsmAnd::IMapStylesPresetsCollection)
%shared_ptr(OsmAnd::MapStyle)
%shared_ptr(OsmAnd::MapStylePreset)

%include <swig/OsmAndCore_Stubs.h>
%include <OsmAndCore/QuickAccessors.h>
%include <OsmAndCore/Callable.h>
%include <OsmAndCore/CommonTypes.h>
%include <OsmAndCore/IObfsCollection.h>
%include <OsmAndCore/ObfDataInterface.h>
%include <OsmAndCore/ResourcesManager.h>
%include <OsmAndCore/Data/DataTypes.h>
%include <OsmAndCore/Data/ObfFile.h>
%include <OsmAndCore/Map/MapTypes.h>
%include <OsmAndCore/Map/IMapStylesCollection.h>
%include <OsmAndCore/Map/IMapStylesPresetsCollection.h>
%include <OsmAndCore/Map/MapStyle.h>
%include <OsmAndCore/Map/MapStylePreset.h>

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
