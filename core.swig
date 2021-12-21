%module(directors="1") OsmAndCore

// Namespaces are useless at this point
//%nspace;

%{
#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/ResourcesManager.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/ObfDataInterface.h>
#include <OsmAndCore/ICoreResourcesProvider.h>
#include <OsmAndCore/CoreResourcesEmbeddedBundle.h>
#include <OsmAndCore/ITypefaceFinder.h>
#include <OsmAndCore/CachingTypefaceFinder.h>
#include <OsmAndCore/ChainedTypefaceFinder.h>
#include <OsmAndCore/EmbeddedTypefaceFinder.h>
#include <OsmAndCore/SystemTypefaceFinder.h>
#include <OsmAndCore/TextRasterizer.h>
#include <OsmAndCore/Logging.h>
#include <OsmAndCore/ILogSink.h>
#include <OsmAndCore/DefaultLogSink.h>
#include <OsmAndCore/QIODeviceLogSink.h>
#include <OsmAndCore/Utilities.h>
#include <OsmAndCore/WorldRegions.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/Data/MapObject.h>
#include <OsmAndCore/Data/ObfMapObject.h>
#include <OsmAndCore/Data/BinaryMapObject.h>
#include <OsmAndCore/Data/Road.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>
#include <OsmAndCore/Data/ObfPoiSectionInfo.h>
#include <OsmAndCore/Data/Amenity.h>
#include <OsmAndCore/Data/ObfAddressSectionInfo.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/Data/Building.h>
#include <OsmAndCore/Data/StreetIntersection.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IBillboardMapSymbol.h>
#include <OsmAndCore/Map/IOnSurfaceMapSymbol.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/OnSurfaceRasterMapSymbol.h>
#include <OsmAndCore/Map/OnSurfaceVectorMapSymbol.h>
#include <OsmAndCore/Map/SymbolRasterizer.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>
#include <OsmAndCore/Map/UnresolvedMapStyle.h>
#include <OsmAndCore/Map/MapStyleConstantValue.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>
#include <OsmAndCore/Map/IMapStyle.h>
#include <OsmAndCore/Map/ResolvedMapStyle.h>
#include <OsmAndCore/Map/MapStylesCollection.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>
#include <OsmAndCore/Map/MapPresentationEnvironment.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>
#include <OsmAndCore/Map/MapRendererDebugSettings.h>
#include <OsmAndCore/Map/MapRendererTypes.h>
#include <OsmAndCore/Map/IMapRenderer.h>
#include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
#include <OsmAndCore/Map/IMapDataProvider.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>
#include <OsmAndCore/Map/IMapKeyedDataProvider.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>
#include <OsmAndCore/Map/IMapLayerProvider.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/ImageMapLayerProvider.h>
#include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
#include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
#include <OsmAndCore/Map/AmenitySymbolsProvider.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>
#include <OsmAndCore/Map/IMapObjectsProvider.h>
#include <OsmAndCore/Map/MapObjectsProvider.h>
#include <OsmAndCore/Map/ObfMapObjectsProvider.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>
#include <OsmAndCore/Map/MapObjectsSymbolsProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Software.h>
#include <OsmAndCore/Map/IOnlineTileSources.h>
#include <OsmAndCore/Map/OnlineTileSources.h>
#include <OsmAndCore/Map/OnlineRasterMapLayerProvider.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>
#include <OsmAndCore/Map/MapMarker.h>
#include <OsmAndCore/Map/MapMarkersCollection.h>
#include <OsmAndCore/Map/MapMarkerBuilder.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/RoadLocator.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Search/ISearch.h>
#include <OsmAndCore/Search/BaseSearch.h>
#include <OsmAndCore/Search/AmenitiesByNameSearch.h>
#include <OsmAndCore/Search/AmenitiesInAreaSearch.h>
#include <OsmAndCore/Search/AddressesByNameSearch.h>
#include <OsmAndCore/Search/ReverseGeocoder.h>
#include "SwigUtilities.h"

// To omit 'OsmAnd::' prefixes inside namespace itself in headers, use that namespace for SWIG
using namespace OsmAnd;
%}

%feature("director") IQueryController;

#if defined(SWIG_JAVA) || defined(SWIG_CSHARP)
	%include <enums.swg>
#endif
%include <typemaps.i>
%include <carrays.i>
#if defined(SWIG_JAVA)
	%include <various.i>
#endif
%include <stdint.i>
%include <cpointer.i>
	%pointer_class(bool, BoolPtr);
	%pointer_class(int, IntPtr);
	%pointer_class(unsigned int, UIntPtr);
	%pointer_class(float, FloatPtr);
	%pointer_class(double, DoublePtr);
%include <std_shared_ptr.i>
    %shared_ptr(OsmAnd::IBillboardMapSymbol)
	%shared_ptr(OsmAnd::ObfInfo)
	%shared_ptr(OsmAnd::IObfsCollection)
	%shared_ptr(OsmAnd::ObfsCollection)
	%shared_ptr(OsmAnd::ObfDataInterface)
	%shared_ptr(OsmAnd::Amenity)
	%shared_ptr(OsmAnd::Address)
	%shared_ptr(OsmAnd::StreetGroup)
	%shared_ptr(OsmAnd::Street)
    %shared_ptr(OsmAnd::Building)
    %shared_ptr(OsmAnd::StreetIntersection)
	%shared_ptr(OsmAnd::ObfSectionInfo)
	%shared_ptr(OsmAnd::ObfPoiSectionInfo)
	%shared_ptr(OsmAnd::ObfPoiSectionCategories)
	%shared_ptr(OsmAnd::ObfPoiSectionSubtypes)
	%shared_ptr(OsmAnd::ObfPoiSectionSubtype)
	%shared_ptr(OsmAnd::ObfAddressSectionInfo)
    %shared_ptr(OsmAnd::IRoadLocator)
    %shared_ptr(OsmAnd::RoadLocator)
	%shared_ptr(OsmAnd::IQueryController)
	%shared_ptr(OsmAnd::ISearch)
	%shared_ptr(OsmAnd::ISearch::Criteria)
	%shared_ptr(OsmAnd::BaseSearch)
	%shared_ptr(OsmAnd::AmenitiesByNameSearch)
	%shared_ptr(OsmAnd::AmenitiesByNameSearch::Criteria)
	%shared_ptr(OsmAnd::AmenitiesInAreaSearch)
	%shared_ptr(OsmAnd::AmenitiesInAreaSearch::Criteria)
	%shared_ptr(OsmAnd::AddressesByNameSearch)
	%shared_ptr(OsmAnd::AddressesByNameSearch::Criteria)
    %shared_ptr(OsmAnd::ReverseGeocoder)
    %shared_ptr(OsmAnd::ReverseGeocoder::Criteria)
	%shared_ptr(OsmAnd::ResourcesManager::Resource)
	%shared_ptr(OsmAnd::ResourcesManager::Resource::Metadata)
	%shared_ptr(OsmAnd::ResourcesManager::BuiltinResource)
	%shared_ptr(OsmAnd::ResourcesManager::LocalResource)
	%shared_ptr(OsmAnd::ResourcesManager::ResourceInRepository)
	%shared_ptr(OsmAnd::ResourcesManager::InstalledResource)
	%shared_ptr(OsmAnd::ResourcesManager::UnmanagedResource)
	%shared_ptr(OsmAnd::ResourcesManager::ObfMetadata)
	%shared_ptr(OsmAnd::ResourcesManager::MapStyleMetadata)
	%shared_ptr(OsmAnd::ResourcesManager::OnlineTileSourcesMetadata)
	%shared_ptr(OsmAnd::ICoreResourcesProvider)
	%shared_ptr(OsmAnd::CoreResourcesEmbeddedBundle)
	%shared_ptr(OsmAnd::ITypefaceFinder)
	%shared_ptr(OsmAnd::CachingTypefaceFinder)
	%shared_ptr(OsmAnd::ChainedTypefaceFinder)
	%shared_ptr(OsmAnd::EmbeddedTypefaceFinder)
	%shared_ptr(OsmAnd::SystemTypefaceFinder)
	%shared_ptr(OsmAnd::TextRasterizer)
	%shared_ptr(OsmAnd::Logger)
	%shared_ptr(OsmAnd::ILogSink)
	%shared_ptr(OsmAnd::DefaultLogSink)
	%shared_ptr(OsmAnd::QIODeviceLogSink)
	%shared_ptr(OsmAnd::ObfFile)
	%shared_ptr(OsmAnd::MapObject)
	%shared_ptr(OsmAnd::ObfMapObject)
	%shared_ptr(OsmAnd::BinaryMapObject)
	%shared_ptr(OsmAnd::Road)
	%shared_ptr(OsmAnd::MapSymbolsGroup)
	%shared_ptr(OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters)
	%shared_ptr(OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters)
	%shared_ptr(OsmAnd::MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters)
	%shared_ptr(OsmAnd::MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters)
	%shared_ptr(OsmAnd::MapSymbol)
	%shared_ptr(OsmAnd::SymbolRasterizer)
	%shared_ptr(OsmAnd::UnresolvedMapStyle)
	%shared_ptr(OsmAnd::UnresolvedMapStyle::RuleNode)
	%shared_ptr(OsmAnd::UnresolvedMapStyle::BaseRule)
	%shared_ptr(OsmAnd::UnresolvedMapStyle::Rule)
	%shared_ptr(OsmAnd::UnresolvedMapStyle::Attribute)
	%shared_ptr(OsmAnd::UnresolvedMapStyle::Parameter)
	%shared_ptr(OsmAnd::MapStyleConstantValue)
	%shared_ptr(OsmAnd::MapStyleValueDefinition)
	%shared_ptr(OsmAnd::IMapStyle)
	%shared_ptr(OsmAnd::IMapStyle::Value)
	%shared_ptr(OsmAnd::IMapStyle::IRuleNode)
	%shared_ptr(OsmAnd::IMapStyle::IRule)
	%shared_ptr(OsmAnd::IMapStyle::IAttribute)
	%shared_ptr(OsmAnd::IMapStyle::IParameter)
	%shared_ptr(OsmAnd::ResolvedMapStyle)
	%shared_ptr(OsmAnd::ResolvedMapStyle::ResolvedValue)
	%shared_ptr(OsmAnd::ResolvedMapStyle::RuleNode)
	%shared_ptr(OsmAnd::ResolvedMapStyle::BaseRule)
	%shared_ptr(OsmAnd::ResolvedMapStyle::Rule)
	%shared_ptr(OsmAnd::ResolvedMapStyle::Attribute)
	%shared_ptr(OsmAnd::ResolvedMapStyle::Parameter)
	%shared_ptr(OsmAnd::ResolvedMapStyle::ParameterValueDefinition)
	%shared_ptr(OsmAnd::IMapStylesCollection)
	%shared_ptr(OsmAnd::MapStylesCollection)
	%shared_ptr(OsmAnd::MapStyleBuiltinValueDefinitions)
	%shared_ptr(OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinition)
	%shared_ptr(OsmAnd::MapPresentationEnvironment)
	%shared_ptr(OsmAnd::MapRendererSetupOptions)
	%shared_ptr(OsmAnd::MapRendererConfiguration)
	%shared_ptr(OsmAnd::MapRendererDebugSettings)
	%shared_ptr(OsmAnd::IMapRenderer)
	%shared_ptr(OsmAnd::AtlasMapRendererConfiguration)
	%shared_ptr(OsmAnd::IMapDataProvider)
	%shared_ptr(OsmAnd::IMapTiledDataProvider)
	%shared_ptr(OsmAnd::IMapKeyedDataProvider)
	%shared_ptr(OsmAnd::IMapElevationDataProvider)
	%shared_ptr(OsmAnd::IMapLayerProvider)
	%shared_ptr(OsmAnd::IRasterMapLayerProvider)
	%shared_ptr(OsmAnd::ImageMapLayerProvider)
	%shared_ptr(OsmAnd::IMapTiledSymbolsProvider)
	%shared_ptr(OsmAnd::IMapKeyedSymbolsProvider)
    %shared_ptr(OsmAnd::AmenitySymbolsProvider)
    %shared_ptr(OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup)
	%shared_ptr(OsmAnd::MapPrimitiviser)
	%shared_ptr(OsmAnd::MapPrimitiviser::CoastlineMapObject)
	%shared_ptr(OsmAnd::MapPrimitiviser::SurfaceMapObject)
	%shared_ptr(OsmAnd::MapPrimitiviser::Cache)
	%shared_ptr(OsmAnd::MapPrimitiviser::Primitive)
	%shared_ptr(OsmAnd::MapPrimitiviser::PrimitivesGroup)
	%shared_ptr(OsmAnd::MapPrimitiviser::Symbol)
	%shared_ptr(OsmAnd::MapPrimitiviser::SymbolsGroup)
	%shared_ptr(OsmAnd::MapPrimitiviser::TextSymbol)
	%shared_ptr(OsmAnd::MapPrimitiviser::IconSymbol)
	%shared_ptr(OsmAnd::MapPrimitiviser::PrimitivisedArea)
	%shared_ptr(OsmAnd::IMapObjectsProvider)
	%shared_ptr(OsmAnd::ObfMapObjectsProvider)
	%shared_ptr(OsmAnd::MapObjectsProvider)
	%shared_ptr(OsmAnd::MapPrimitivesProvider)
	%shared_ptr(OsmAnd::MapObjectsSymbolsProvider)
	%shared_ptr(OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup)
	%shared_ptr(OsmAnd::MapRasterLayerProvider)
	%shared_ptr(OsmAnd::MapRasterLayerProvider_Software)
	%shared_ptr(OsmAnd::IOnlineTileSources)
	%shared_ptr(OsmAnd::IOnlineTileSources::Source)
	%shared_ptr(OsmAnd::OnlineTileSources)
	%shared_ptr(OsmAnd::OnlineRasterMapLayerProvider)
	%shared_ptr(OsmAnd::MapMarker)
	%shared_ptr(OsmAnd::MapMarker::SymbolsGroup)
	%shared_ptr(OsmAnd::MapMarkersCollection)
%include <std_pair.i>
%include <std_string.i>
#ifdef SWIG_JAVA
	%include <swig/java/QString.i>
		%apply QString { std::string };
#endif
%include <std_vector.i>
#ifdef SWIG_JAVA
	%include <swig/java/QList.i>
		%template(QStringList) QList<QString>;
	%include <swig/java/QVector.i>
#endif
%include <std_map.i>
#ifdef SWIG_JAVA
	%include <swig/java/QHash.i>
	%include <swig/java/QMap.i>
#endif
%template(QStringStringHash) QHash<QString, QString>;
%template(QStringStringMap) QMap<QString, QString>;
%template(QVariantIntHash) QHash<int, QVariant>;
%template(QStringStringListHash) QHash<QString, QStringList>;

#ifdef SWIG_JAVA
	%include <swig/java/byteArrays.i>
#endif

#ifdef SWIG_JAVA
	%rename(equals) operator==;
	%rename(notEquals) operator!=;
	%rename(method) operator();
#endif
%include <swig/OsmAndCore_Stubs.h>
%include <swig/SwigUtilities.h>
%include <OsmAndCore.h>
%include <OsmAndCore/CommonSWIG.h>
%include <OsmAndCore/QuickAccessors.h>
%include <OsmAndCore/Observable.h>
%include <OsmAndCore/Callable.h>
%include <OsmAndCore/CommonTypes.h>
%include <OsmAndCore/Color.h>
%include <OsmAndCore/PointsAndAreas.h>
	%template(PointI) OsmAnd::Point<int32_t>;
//	%template(PointI64) OsmAnd::Point<int64_t>;
	%template(PointF) OsmAnd::Point<float>;
	%template(PointD) OsmAnd::Point<double>;
	%template(AreaI) OsmAnd::Area<int32_t>;
//	%template(AreaI64) OsmAnd::Area<int64_t>;
	%template(AreaF) OsmAnd::Area<float>;
	%template(AreaD) OsmAnd::Area<double>;
	%template(OOBBI) OsmAnd::OOBB<int32_t>;
//	%template(OOBBI64) OsmAnd::OOBB<int64_t>;
	%template(OOBBF) OsmAnd::OOBB<float>;
	%template(OOBBD) OsmAnd::OOBB<double>;
%include <OsmAndCore/LatLon.h>
%include <OsmAndCore/Nullable.h>
	%template(NullableAreaI) OsmAnd::Nullable<OsmAnd::Area<int32_t>>;
    %template(NullablePointI) OsmAnd::Nullable<OsmAnd::Point<int32_t>>;
    %template(NullableLatLon) OsmAnd::Nullable<OsmAnd::LatLon>;
%include <OsmAndCore/Bitmask.h>
	%template(MapRendererStateChanges) OsmAnd::Bitmask<OsmAnd::MapRendererStateChange, unsigned int>;
	%template(ObfDataTypesMask) OsmAnd::Bitmask<OsmAnd::ObfDataType, unsigned int>;
	%template(ObfAddressStreetGroupTypesMask) OsmAnd::Bitmask<OsmAnd::ObfAddressStreetGroupType, unsigned int>;
%include <OsmAndCore/IObfsCollection.h>
%include <OsmAndCore/ObfsCollection.h>
%include <OsmAndCore/ObfDataInterface.h>
%include <OsmAndCore/ResourcesManager.h>
%include <OsmAndCore/ICoreResourcesProvider.h>
%include <OsmAndCore/CoreResourcesEmbeddedBundle.h>
%include <OsmAndCore/ITypefaceFinder.h>
%include <OsmAndCore/CachingTypefaceFinder.h>
%include <OsmAndCore/ChainedTypefaceFinder.h>
%include <OsmAndCore/EmbeddedTypefaceFinder.h>
%include <OsmAndCore/SystemTypefaceFinder.h>
%include <OsmAndCore/TextRasterizer.h>
%include <OsmAndCore/Logging.h>
%include <OsmAndCore/ILogSink.h>
%include <OsmAndCore/DefaultLogSink.h>
%include <OsmAndCore/QIODeviceLogSink.h>
%include <OsmAndCore/Utilities.h>
%include <OsmAndCore/WorldRegions.h>
%include <OsmAndCore/Data/DataCommonTypes.h>
%include <OsmAndCore/Data/ObfFile.h>
%include <OsmAndCore/Data/ObfInfo.h>
%include <OsmAndCore/Data/MapObject.h>
%include <OsmAndCore/Data/ObfMapObject.h>
%include <OsmAndCore/Data/BinaryMapObject.h>
%include <OsmAndCore/Data/Road.h>
%include <OsmAndCore/Data/ObfSectionInfo.h>
%include <OsmAndCore/Data/ObfPoiSectionInfo.h>
%include <OsmAndCore/Data/Amenity.h>
	%template(ObfPoiCategoryIdList) QList<OsmAnd::ObfPoiCategoryId>;
	%template(DecodedCategoryList) QList<OsmAnd::Amenity::DecodedCategory>;
	%template(DecodedValueList) QList<OsmAnd::Amenity::DecodedValue>;
%include <OsmAndCore/Data/ObfAddressSectionInfo.h>
%include <OsmAndCore/Data/Address.h>
%include <OsmAndCore/Data/StreetGroup.h>
%include <OsmAndCore/Data/Street.h>
%include <OsmAndCore/Data/Building.h>
%include <OsmAndCore/Data/StreetIntersection.h>
%include <OsmAndCore/Data/ObfRoutingSectionReader.h>
%include <OsmAndCore/Map/MapCommonTypes.h>
%include <OsmAndCore/Map/MapSymbol.h>
%include <OsmAndCore/Map/IBillboardMapSymbol.h>
%include <OsmAndCore/Map/IOnSurfaceMapSymbol.h>
%include <OsmAndCore/Map/MapSymbolsGroup.h>
%include <OsmAndCore/Map/SymbolRasterizer.h>
%include <OsmAndCore/Map/UnresolvedMapStyle.h>
%include <OsmAndCore/Map/MapStyleConstantValue.h>
%include <OsmAndCore/Map/MapStyleValueDefinition.h>
%include <OsmAndCore/Map/IMapStyle.h>
%include <OsmAndCore/Map/ResolvedMapStyle.h>
%include <OsmAndCore/Map/IMapStylesCollection.h>
%include <OsmAndCore/Map/MapStylesCollection.h>
%include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>
%include <OsmAndCore/Map/MapPresentationEnvironment.h>
%include <OsmAndCore/Map/MapRendererState.h>
%include <OsmAndCore/Map/MapRendererSetupOptions.h>
%include <OsmAndCore/Map/MapRendererConfiguration.h>
%include <OsmAndCore/Map/MapRendererDebugSettings.h>
%include <OsmAndCore/Map/MapRendererTypes.h>
%include <OsmAndCore/Map/IMapRenderer.h>
%include <OsmAndCore/Map/AtlasMapRendererConfiguration.h>
%include <OsmAndCore/Map/IMapDataProvider.h>
%include <OsmAndCore/Map/IMapTiledDataProvider.h>
%include <OsmAndCore/Map/IMapKeyedDataProvider.h>
%include <OsmAndCore/Map/IMapElevationDataProvider.h>
%include <OsmAndCore/Map/IMapLayerProvider.h>
%include <OsmAndCore/Map/IRasterMapLayerProvider.h>
%include <OsmAndCore/Map/ImageMapLayerProvider.h>
%include <OsmAndCore/Map/IMapTiledSymbolsProvider.h>
%include <OsmAndCore/Map/IMapKeyedSymbolsProvider.h>
%include <OsmAndCore/Map/AmenitySymbolsProvider.h>
%include <OsmAndCore/Map/MapPrimitiviser.h>
%include <OsmAndCore/Map/IMapObjectsProvider.h>
%include <OsmAndCore/Map/ObfMapObjectsProvider.h>
%include <OsmAndCore/Map/MapPrimitivesProvider.h>
%include <OsmAndCore/Map/MapObjectsSymbolsProvider.h>
%include <OsmAndCore/Map/MapRasterLayerProvider.h>
%include <OsmAndCore/Map/MapRasterLayerProvider_Software.h>
%include <OsmAndCore/Map/IOnlineTileSources.h>
%include <OsmAndCore/Map/OnlineTileSources.h>
%include <OsmAndCore/Map/OnlineRasterMapLayerProvider.h>
%include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>
%include <OsmAndCore/Map/MapMarker.h>
%include <OsmAndCore/Map/MapMarkersCollection.h>
%include <OsmAndCore/Map/MapMarkerBuilder.h>
	%template(MapSymbolInformationList) QList<OsmAnd::IMapRenderer::MapSymbolInformation>;
%include <OsmAndCore/IRoadLocator.h>
%include <OsmAndCore/RoadLocator.h>
%include <OsmAndCore/IQueryController.h>
%include <OsmAndCore/Search/ISearch.h>
%include <OsmAndCore/Search/BaseSearch.h>
%include <OsmAndCore/Search/AmenitiesByNameSearch.h>
%include <OsmAndCore/Search/AmenitiesInAreaSearch.h>
%include <OsmAndCore/Search/AddressesByNameSearch.h>
%include <OsmAndCore/Search/ReverseGeocoder.h>

%exception OsmAnd::MapMarker::SymbolsGroup::dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::MapMarker::SymbolsGroup {
    static OsmAnd::MapMarker::SymbolsGroup *dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
        return dynamic_cast<OsmAnd::MapMarker::SymbolsGroup *>(symbolsGroup);
    }
};

%exception OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup {
    static OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup *dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
        return dynamic_cast<OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup *>(symbolsGroup);
    }
};

%exception OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup {
    static OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup *dynamic_cast(OsmAnd::MapSymbolsGroup *symbolsGroup) {
        return dynamic_cast<OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup *>(symbolsGroup);
    }
};

%exception OsmAnd::IBillboardMapSymbol::dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapSymbol> mapSymbol) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::IBillboardMapSymbol {
    static std::shared_ptr<OsmAnd::IBillboardMapSymbol> dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapSymbol> mapSymbol) {
        return std::dynamic_pointer_cast<OsmAnd::IBillboardMapSymbol>(mapSymbol);
    }
};

%exception OsmAnd::ObfMapObject::dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapObject> mapObject) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::ObfMapObject {
    static std::shared_ptr<OsmAnd::ObfMapObject> dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapObject> mapObject) {
        return std::dynamic_pointer_cast<OsmAnd::ObfMapObject>(mapObject);
    }
};

%exception OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters::dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters> instanceParameters) {
$action
    if (!result) {
        jclass excep = jenv->FindClass("java/lang/ClassCastException");
        if (excep) {
            jenv->ThrowNew(excep, "dynamic_cast exception");
        }
    }
}
%extend OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters {
    static std::shared_ptr<OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters> dynamic_pointer_cast(std::shared_ptr<OsmAnd::MapSymbolsGroup::AdditionalSymbolInstanceParameters> instanceParameters) {
        return std::dynamic_pointer_cast<OsmAnd::MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>(instanceParameters);
    }
};

