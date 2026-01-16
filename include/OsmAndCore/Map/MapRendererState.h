#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>
#include <bitset>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QSet>
#include <QMap>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>

SWIG_TEMPLATE(MapLayerProvidersMap, QMap<int, std::shared_ptr<OsmAnd::IMapLayerProvider> >);
SWIG_TEMPLATE(MapLayerConfigurationsMap, QMap<int, MapLayerConfiguration>);

namespace OsmAnd
{
    class IMapLayerProvider;
    class IMapElevationDataProvider;
    class IMapTiledSymbolsProvider;
    class IMapKeyedSymbolsProvider;
    class IMapTiledDataProvider;
    class IMapKeyedDataProvider;
    class IMapRenderer;
    class MapRenderer;

    enum class MapRendererStateChange
    {
        MapLayers_Providers = 0,
        MapLayers_Configuration,
        Elevation_DataProvider,
        Elevation_Configuration,
        Grid_Configuration,
        Symbols_Providers,
        Symbols_Configuration,
        Map3DObjects_Provider,
        WindowSize,
        Viewport,
        Flip,
        FlatEarth,
        FieldOfView,
        VisibleDistance,
        DetailedDistance,
        SkyColor,
        Azimuth,
        ElevationAngle,
        Target,
        Zoom,
        StubsStyle,
        BackgroundColor,
        FogColor,
        MyLocation,
        SymbolsOpacity,
        DateTime,
        TimePeriod
    };
    typedef Bitmask<MapRendererStateChange> MapRendererStateChanges;

    enum AimingAction
    {
        NoAction = 0,
        Azimuth,
        Zoom,
        Elevation,
    };
    typedef std::bitset<8 * sizeof(int)> AimingActions;

    struct OSMAND_CORE_API MapState Q_DECL_FINAL
    {
        MapState();
        ~MapState();
        
        PointI windowSize;
        AreaI viewport;
        bool flip;
        bool flatEarth;
        float fieldOfView;
        float visibleDistance;
        float detailedDistance;
        FColorRGB skyColor;
        float azimuth;
        float elevationAngle;
        PointI target31;
        PointI fixedPixel;
        PointI fixedLocation31;
        float fixedHeight;
        ZoomLevel fixedZoomLevel;
        PointI aimPixel;
        PointI aimLocation31;
        float aimHeight;
        ZoomLevel aimZoomLevel;
        AimingActions aimingActions;
        ZoomLevel zoomLevel;
        float visualZoom;
        ZoomLevel surfaceZoomLevel;
        float surfaceVisualZoom;
        float visualZoomShift;
        float extraScale;
        ZoomLevel minZoomLimit;
        ZoomLevel maxZoomLimit;
        MapStubStyle stubsStyle;
        FColorRGB backgroundColor;
        FColorRGB fogColor;
        FColorARGB myLocationColor;
        PointI myLocation31;
        float myLocationRadiusInMeters;
        float myDirection;
        float myDirectionRadius;
        float symbolsOpacity;
        int64_t dateTime;
        
        double metersPerPixel;
        AreaI visibleBBox31;
        AreaI visibleBBoxShifted;
        bool hasElevationDataProvider;
        bool hasElevationDataResources;
    };

    struct OSMAND_CORE_API MapRendererState Q_DECL_FINAL
    {
        MapRendererState();
        ~MapRendererState();

        MapState getMapState() const; 
        void getGridConfiguration(GridConfiguration* gridConfiguration, ZoomLevel* zoomLevel) const;
        ZoomLevel getVisibleArea(AreaI* visibleBBoxShifted, PointI* target31) const;
    
        QMap<int, std::shared_ptr<IMapLayerProvider> > mapLayersProviders;
        QMap<int, MapLayerConfiguration > mapLayersConfigurations;

        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        ElevationConfiguration elevationConfiguration;
        GridConfiguration gridConfiguration;

        QHash<int, QSet<std::shared_ptr<IMapTiledSymbolsProvider>>> tiledSymbolsProviders;
        QHash<std::shared_ptr<IMapTiledSymbolsProvider>, int> tiledSymbolsSubsections;
        QHash<int, QSet<std::shared_ptr<IMapKeyedSymbolsProvider>>> keyedSymbolsProviders;
        QHash<std::shared_ptr<IMapKeyedSymbolsProvider>, int> keyedSymbolsSubsections;
        QMap<int, SymbolSubsectionConfiguration > symbolSubsectionConfigurations;

        std::shared_ptr<IMapTiledDataProvider> map3DObjectsProvider;

        mutable volatile bool isChanged;

        PointI windowSize;
        AreaI viewport;
        bool flip;
        bool flatEarth;
        float fieldOfView;
        float visibleDistance;
        float detailedDistance;
        FColorRGB skyColor;
        float azimuth;
        float elevationAngle;
        float elevationAngleToBe;
        PointI target31;
        PointI fixedPixel;
        PointI fixedLocation31;
        float fixedHeight;
        ZoomLevel fixedZoomLevel;
        PointI aimPixel;
        PointI aimLocation31;
        float aimHeight;
        ZoomLevel aimZoomLevel;
        AimingActions aimingActions;
        ZoomLevel zoomLevel;
        float visualZoom;
        ZoomLevel surfaceZoomLevel;
        float surfaceVisualZoom;
        ZoomLevel surfaceZoomLevelToBe;
        float surfaceVisualZoomToBe;
        float visualZoomShift;
        float extraScale;
        ZoomLevel minZoomLimit;
        ZoomLevel maxZoomLimit;
        MapStubStyle stubsStyle;
        FColorRGB backgroundColor;
        FColorRGB fogColor;
        FColorARGB myLocationColor;
        PointI myLocation31;
        float myLocationRadiusInMeters;
        float myDirection;
        float myDirectionRadius;
        float symbolsOpacity;
        int64_t dateTime;
        
        double metersPerPixel;
        AreaI visibleBBox31;
        AreaI visibleBBoxShifted;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STATE_H_)
