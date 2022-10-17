#ifndef _OSMAND_CORE_I_ATLAS_MAP_RENDERER_H_
#define _OSMAND_CORE_I_ATLAS_MAP_RENDERER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/Map/IMapRenderer.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IAtlasMapRenderer
    {
        Q_DISABLE_COPY_AND_MOVE(IAtlasMapRenderer);

    private:
    protected:
        IAtlasMapRenderer();
    public:
        virtual ~IAtlasMapRenderer();

        virtual QVector<TileId> getVisibleTiles() const = 0;
        virtual unsigned int getVisibleTilesCount() const = 0;
        virtual unsigned int getAllTilesCount() const = 0;
        virtual unsigned int getDetailLevelsCount() const = 0;
        virtual LatLon getCameraCoordinates() const = 0;
        virtual double getCameraHeight() const = 0;

        virtual int getTileSize3D() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ATLAS_MAP_RENDERER_H_)
