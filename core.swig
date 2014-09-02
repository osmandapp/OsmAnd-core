%module(directors="1") OsmAndCore

// Namespaces are useless at this point
//%nspace;

%{
#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/ResourcesManager.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/IExternalResourcesProvider.h>
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/MapStyleRule.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>
#include <OsmAndCore/Map/MapStyleValue.h>
#include <OsmAndCore/Map/IMapStylesPresetsCollection.h>
#include <OsmAndCore/Map/MapStylesPresetsCollection.h>
#include <OsmAndCore/Map/MapStylePreset.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapRendererTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>
#include <OsmAndCore/Map/MapRendererDebugSettings.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/IMapDataProvider.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>
#include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/Primitiviser.h>
#include <OsmAndCore/Map/BinaryMapDataProvider.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider.h>
#include <OsmAndCore/Map/BinaryMapStaticSymbolsProvider.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider_Software.h>
#include <OsmAndCore/Map/IOnlineTileSources.h>
#include <OsmAndCore/Map/OnlineTileSources.h>
#include <OsmAndCore/Map/OnlineRasterMapTileProvider.h>

// To omit 'OsmAnd::' prefixes inside namespace itself in headers, use that namespace for SWIG
using namespace OsmAnd;
%}

/*
%include <cpointer.i>
	%pointer_class(bool, BoolPtr);
	%pointer_class(int, IntPtr);
	%pointer_class(unsigned int, UIntPtr);
	%pointer_class(float, FloatPtr);
	%pointer_class(double, DoublePtr);
*/
%include <enums.swg>
%include <typemaps.i>
%include <stdint.i>
%include <std_shared_ptr.i>
	%shared_ptr(OsmAnd::IObfsCollection)
	%shared_ptr(OsmAnd::ObfsCollection)
	%shared_ptr(OsmAnd::ObfDataInterface)
	%shared_ptr(OsmAnd::ResourcesManager::Resource)
	%shared_ptr(OsmAnd::ResourcesManager::Resource::Metadata)
	%shared_ptr(OsmAnd::ResourcesManager::BuiltinResource)
	%shared_ptr(OsmAnd::ResourcesManager::LocalResource)
	%shared_ptr(OsmAnd::ResourcesManager::ResourceInRepository)
	%shared_ptr(OsmAnd::ResourcesManager::InstalledResource)
	%shared_ptr(OsmAnd::ResourcesManager::UnmanagedResource)
	%shared_ptr(OsmAnd::ResourcesManager::ObfMetadata)
	%shared_ptr(OsmAnd::ResourcesManager::MapStyleMetadata)
	%shared_ptr(OsmAnd::ResourcesManager::MapStylesPresetsMetadata)
	%shared_ptr(OsmAnd::ResourcesManager::OnlineTileSourcesMetadata)
	%shared_ptr(OsmAnd::IExternalResourcesProvider)
	%shared_ptr(OsmAnd::ObfFile)
	%shared_ptr(OsmAnd::ObfInfo)
	%shared_ptr(OsmAnd::MapSymbolsGroup)
	%shared_ptr(OsmAnd::MapSymbol)
	%shared_ptr(OsmAnd::IMapStylesCollection)
	%shared_ptr(OsmAnd::MapStylesCollection)
	%shared_ptr(OsmAnd::IMapStylesPresetsCollection)
	%shared_ptr(OsmAnd::MapStylesPresetsCollection)
	%shared_ptr(OsmAnd::MapStyleBuiltinValueDefinitions)
	%shared_ptr(OsmAnd::MapStyle)
	%shared_ptr(OsmAnd::MapStyleRule)
	%shared_ptr(OsmAnd::MapStyleValueDefinition)
	%shared_ptr(OsmAnd::MapStyleValue)
	%shared_ptr(OsmAnd::MapStylePreset)
	%shared_ptr(OsmAnd::MapPresentationEnvironment)
	%shared_ptr(OsmAnd::MapRendererSetupOptions)
	%shared_ptr(OsmAnd::MapRendererConfiguration)
	%shared_ptr(OsmAnd::MapRendererDebugSettings)
	%shared_ptr(OsmAnd::IMapRenderer)
	%shared_ptr(OsmAnd::AtlasMapRendererConfiguration)
	%shared_ptr(OsmAnd::IMapDataProvider)
	%shared_ptr(OsmAnd::MapData)
	%shared_ptr(OsmAnd::IMapTiledDataProvider)
	%shared_ptr(OsmAnd::MapTiledData)
	%shared_ptr(OsmAnd::IMapElevationDataProvider)
	%shared_ptr(OsmAnd::ElevationDataTile)
	%shared_ptr(OsmAnd::IMapRasterBitmapTileProvider)
	%shared_ptr(OsmAnd::RasterBitmapTile)
	%shared_ptr(OsmAnd::IMapTiledSymbolsProvider)
	%shared_ptr(OsmAnd::TiledMapSymbolsData)
	%shared_ptr(OsmAnd::IMapKeyedDataProvider)
	%shared_ptr(OsmAnd::MapKeyedData)
	%shared_ptr(OsmAnd::IMapKeyedSymbolsProvider)
	%shared_ptr(OsmAnd::KeyedMapSymbolsData)
	%shared_ptr(OsmAnd::Primitiviser)
	%shared_ptr(OsmAnd::Primitiviser::Cache)
	%shared_ptr(OsmAnd::Primitiviser::Primitive)
	%shared_ptr(OsmAnd::Primitiviser::PrimitivesGroup)
	%shared_ptr(OsmAnd::Primitiviser::Symbol)
	%shared_ptr(OsmAnd::Primitiviser::SymbolsGroup)
	%shared_ptr(OsmAnd::Primitiviser::TextSymbol)
	%shared_ptr(OsmAnd::Primitiviser::IconSymbol)
	%shared_ptr(OsmAnd::Primitiviser::PrimitivisedArea)
	%shared_ptr(OsmAnd::BinaryMapDataProvider)
	%shared_ptr(OsmAnd::BinaryMapDataTile)
	%shared_ptr(OsmAnd::BinaryMapPrimitivesProvider)
	%shared_ptr(OsmAnd::BinaryMapPrimitivesTile)
	%shared_ptr(OsmAnd::BinaryMapStaticSymbolsProvider)
	%shared_ptr(OsmAnd::BinaryMapStaticSymbolsTile)
	%shared_ptr(OsmAnd::BinaryMapRasterBitmapTileProvider)
	%shared_ptr(OsmAnd::BinaryMapRasterizedTile)
	%shared_ptr(OsmAnd::BinaryMapRasterBitmapTileProvider_Software)
	%shared_ptr(OsmAnd::IOnlineTileSources)
	%shared_ptr(OsmAnd::IOnlineTileSources::Source)
	%shared_ptr(OsmAnd::OnlineTileSources)
	%shared_ptr(OsmAnd::OnlineRasterMapTileProvider)
%include <std_pair.i>
%include <std_string.i>
%include <swig/QString.i>
	%apply QString { std::string };
%include <std_vector.i>
%include <swig/QList.i>
	%template(QStringList) QList<QString>;
%include <swig/QVector.i>

%include <swig/OsmAndCore_Stubs.h>
%include <OsmAndCore.h>
%include <OsmAndCore/QuickAccessors.h>
%include <OsmAndCore/Observable.h>
%include <OsmAndCore/Callable.h>
%include <OsmAndCore/CommonTypes.h>
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
%include <OsmAndCore/Bitmask.h>
	%template(MapRendererStateChanges) OsmAnd::Bitmask<OsmAnd::MapRendererStateChange, unsigned int>;
%include <OsmAndCore/IObfsCollection.h>
%include <OsmAndCore/ObfsCollection.h>
%include <OsmAndCore/ObfDataInterface.h>
%include <OsmAndCore/ResourcesManager.h>
%include <OsmAndCore/IExternalResourcesProvider.h>
%include <OsmAndCore/Data/DataTypes.h>
%include <OsmAndCore/Data/ObfFile.h>
%include <OsmAndCore/Data/ObfInfo.h>
%include <OsmAndCore/Map/MapTypes.h>
%include <OsmAndCore/Map/MapSymbol.h>
%include <OsmAndCore/Map/MapSymbolsGroup.h>
%include <OsmAndCore/Map/IMapStylesCollection.h>
%include <OsmAndCore/Map/MapStylesCollection.h>
%include <OsmAndCore/Map/IMapStylesPresetsCollection.h>
%include <OsmAndCore/Map/MapStylesPresetsCollection.h>
%include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>
%include <OsmAndCore/Map/MapStyle.h>
%include <OsmAndCore/Map/MapStyleRule.h>
%include <OsmAndCore/Map/MapStyleValueDefinition.h>
%include <OsmAndCore/Map/MapStyleValue.h>
%include <OsmAndCore/Map/MapStylePreset.h>
%include <OsmAndCore/Map/MapPresentationEnvironment.h>
%include <OsmAndCore/Map/MapRendererTypes.h>
%include <OsmAndCore/Map/MapRendererState.h>
%include <OsmAndCore/Map/MapRendererSetupOptions.h>
%include <OsmAndCore/Map/MapRendererConfiguration.h>
%include <OsmAndCore/Map/MapRendererDebugSettings.h>
%include <OsmAndCore/Map/IMapRenderer.h>
%include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
%include <OsmAndCore/Map/IMapDataProvider.h>
%include <OsmAndCore/Map/IMapTiledDataProvider.h>
%include <OsmAndCore/Map/IMapElevationDataProvider.h>
%include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>
%include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
%include <OsmAndCore/Map/IMapKeyedDataProvider.h>
%include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
%include <OsmAndCore/Map/Primitiviser.h>
%include <OsmAndCore/Map/BinaryMapDataProvider.h>
%include <OsmAndCore/Map/BinaryMapPrimitivesProvider.h>
%include <OsmAndCore/Map/BinaryMapStaticSymbolsProvider.h>
%include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider.h>
%include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider_Software.h>
%include <OsmAndCore/Map/IOnlineTileSources.h>
%include <OsmAndCore/Map/OnlineTileSources.h>
%include <OsmAndCore/Map/OnlineRasterMapTileProvider.h>
