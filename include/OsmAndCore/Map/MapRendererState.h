#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>

SWIG_TEMPLATE(MapLayerProvidersMap, QMap<int, std::shared_ptr<IMapLayerProvider> >);
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
        ElevationData_Provider,
        ElevationData_Configuration,
        Symbols_Providers,
        WindowSize,
        Viewport,
        FieldOfView,
        SkyColor,
        FogConfiguration,
        Azimuth,
        ElevationAngle,
        Target,
        Zoom,
    };
    typedef Bitmask<MapRendererStateChange> MapRendererStateChanges;

    struct OSMAND_CORE_API MapRendererState Q_DECL_FINAL
    {
        MapRendererState();
        ~MapRendererState();

        QMap<int, std::shared_ptr<IMapLayerProvider> > mapLayersProviders;
        QMap<int, MapLayerConfiguration > mapLayersConfigurations;

        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        ElevationDataConfiguration elevationDataConfiguration;

        QSet< std::shared_ptr<IMapTiledSymbolsProvider> > tiledSymbolsProviders;
        QSet< std::shared_ptr<IMapKeyedSymbolsProvider> > keyedSymbolsProviders;

        PointI windowSize;
        AreaI viewport;
        float fieldOfView;
        FColorRGB skyColor;
        FogConfiguration fogConfiguration;
        float azimuth;
        float elevationAngle;
        PointI target31;
        float requestedZoom;
        ZoomLevel zoomBase;
        float zoomFraction;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STATE_H_)
