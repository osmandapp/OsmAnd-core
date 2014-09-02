#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>

namespace OsmAnd
{
    class IMapDataProvider;
    class IMapRasterBitmapTileProvider;
    class IMapElevationDataProvider;
    class IMapRenderer;
    class MapRenderer;

    enum class MapRendererStateChange
    {
        RasterLayers_Providers = 0,
        RasterLayers_Opacity,
        ElevationData_Provider,
        ElevationData_ScaleFactor,
        Symbols_Providers,
        WindowSize,
        Viewport,
        FieldOfView,
        SkyColor,
        FogParameters,
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

        std::array< std::shared_ptr<IMapRasterBitmapTileProvider>, RasterMapLayersCount > rasterLayerProviders;
        std::array< float, RasterMapLayersCount > rasterLayerOpacity;
        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        float elevationDataScaleFactor;
        QSet< std::shared_ptr<IMapDataProvider> > symbolProviders;
        PointI windowSize;
        AreaI viewport;
        float fieldOfView;
        FColorRGB skyColor;
        FColorRGB fogColor;
        float fogDistance;
        float fogOriginFactor;
        float fogHeightOriginFactor;
        float fogDensity;
        float azimuth;
        float elevationAngle;
        PointI target31;
        float requestedZoom;
        ZoomLevel zoomBase;
        float zoomFraction;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STATE_H_)
