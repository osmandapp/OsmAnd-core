#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>

namespace OsmAnd
{
    class IMapBitmapTileProvider;
    class IMapElevationDataProvider;
    class IMapSymbolProvider;
    class IMapRenderer;
    class MapRenderer;

    enum class MapRendererStateChange : uint32_t
    {
        RasterLayers_Providers = 1 << 0,
        RasterLayers_Opacity = 1 << 1,
        ElevationData_Provider = 1 << 2,
        ElevationData_ScaleFactor = 1 << 3,
        Symbols_Providers = 1 << 4,
        WindowSize = 1 << 5,
        Viewport = 1 << 6,
        FieldOfView = 1 << 7,
        SkyColor = 1 << 8,
        FogParameters = 1 << 9,
        Azimuth = 1 << 10,
        ElevationAngle = 1 << 11,
        Target = 1 << 12,
        Zoom = 1 << 13,
    };

    class OSMAND_CORE_API MapRendererState
    {
    private:
    protected:
        MapRendererState();
    public:
        virtual ~MapRendererState();

        std::array< std::shared_ptr<IMapBitmapTileProvider>, RasterMapLayersCount > rasterLayerProviders;
        std::array< float, RasterMapLayersCount > rasterLayerOpacity;
        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        float elevationDataScaleFactor;
        QSet< std::shared_ptr<IMapSymbolProvider> > symbolProviders;
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

    friend class OsmAnd::IMapRenderer;
    friend class OsmAnd::MapRenderer;
    };

}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STATE_H_)
