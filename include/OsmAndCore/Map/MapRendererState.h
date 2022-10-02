#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

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
    class IMapRenderer;
    class MapRenderer;

    enum class MapRendererStateChange
    {
        MapLayers_Providers = 0,
        MapLayers_Configuration,
        Elevation_DataProvider,
        Elevation_Configuration,
        Symbols_Providers,
        Symbols_Configuration,
        WindowSize,
        Viewport,
        FieldOfView,
        SkyColor,
        FogConfiguration,
        Azimuth,
        ElevationAngle,
        Target,
        Zoom,
        StubsStyle,
        BackgroundColor,
        SymbolsOpacity
    };
    typedef Bitmask<MapRendererStateChange> MapRendererStateChanges;

    struct OSMAND_CORE_API MapState Q_DECL_FINAL
    {
        MapState();
        ~MapState();
        
        PointI windowSize;
        AreaI viewport;
        float fieldOfView;
        FColorRGB skyColor;
        FogConfiguration fogConfiguration;
        float azimuth;
        float elevationAngle;
        PointI target31;
        PointI fixedPixel;
        PointI fixedLocation31;
        float fixedHeight;
        ZoomLevel fixedZoomLevel;
        ZoomLevel zoomLevel;
        float visualZoom;
        float visualZoomShift;
        MapStubStyle stubsStyle;
        FColorRGB backgroundColor;
        float symbolsOpacity;
        
        double metersPerPixel;
        AreaI visibleBBox31;
        bool hasElevationDataProvider;
    };

    struct OSMAND_CORE_API MapRendererState Q_DECL_FINAL
    {
        MapRendererState();
        ~MapRendererState();

        MapState getMapState() const; 

        QMap<int, std::shared_ptr<IMapLayerProvider> > mapLayersProviders;
        QMap<int, MapLayerConfiguration > mapLayersConfigurations;

        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        ElevationConfiguration elevationConfiguration;

        QMap<int, QSet< std::shared_ptr<IMapTiledSymbolsProvider> > > tiledSymbolsProviders;
        QMap<int, QSet< std::shared_ptr<IMapKeyedSymbolsProvider> > > keyedSymbolsProviders;
        QMap<int, SymbolSubsectionConfiguration > symbolSubsectionConfigurations;

        PointI windowSize;
        AreaI viewport;
        float fieldOfView;
        FColorRGB skyColor;
        FogConfiguration fogConfiguration;
        float azimuth;
        float elevationAngle;
        PointI target31;
        PointI fixedPixel;
        PointI fixedLocation31;
        float fixedHeight;
        ZoomLevel fixedZoomLevel;
        ZoomLevel zoomLevel;
        float visualZoom;
        float visualZoomShift;
        MapStubStyle stubsStyle;
        FColorRGB backgroundColor;
        float symbolsOpacity;
        
        double metersPerPixel;
        AreaI visibleBBox31;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STATE_H_)
