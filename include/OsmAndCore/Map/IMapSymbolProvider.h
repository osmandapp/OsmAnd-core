/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapProvider.h>
#include <OsmAndCore/Map/IRetainableResource.h>

class SkBitmap;

namespace OsmAnd
{
    namespace Model {
        class MapObject;
    } // namespace Model

    class MapSymbol;
    class OSMAND_CORE_API MapSymbolsGroup
    {
        Q_DISABLE_COPY(MapSymbolsGroup);
    private:
    protected:
    public:
        MapSymbolsGroup(const std::shared_ptr<const Model::MapObject>& mapObject);
        virtual ~MapSymbolsGroup();

        const std::shared_ptr<const Model::MapObject> mapObject;
        QList< std::shared_ptr<const MapSymbol> > symbols;
    };

    class OSMAND_CORE_API MapSymbol
        : public IRetainableResource
    {
        Q_DISABLE_COPY(MapSymbol);
    private:
    protected:
        std::shared_ptr<const SkBitmap> _bitmap;
    public:
        MapSymbol(const std::weak_ptr<const MapSymbolsGroup>& group, const std::shared_ptr<const Model::MapObject>& mapObject, const int order, const PointI& location31, const std::shared_ptr<const SkBitmap>& bitmap);
        virtual ~MapSymbol();

        const std::weak_ptr<const MapSymbolsGroup> group;

        const std::shared_ptr<const Model::MapObject> mapObject;
        const int order;
        const PointI location31;
        const std::shared_ptr<const SkBitmap>& bitmap;

        virtual void releaseNonRetainedData();
    };

    class OSMAND_CORE_API MapSymbolsTile
    {
    private:
    protected:
        MapSymbolsTile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups);

        QList< std::shared_ptr<const MapSymbolsGroup> > _symbolsGroups;
    public:
        virtual ~MapSymbolsTile();

        const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups;
    };

    class OSMAND_CORE_API IMapSymbolProvider : public IMapProvider
    {
    private:
    protected:
        IMapSymbolProvider();
    public:
        virtual ~IMapSymbolProvider();

        virtual bool obtainSymbols(
            const TileId tileId, const ZoomLevel zoom,
            std::shared_ptr<const MapSymbolsTile>& outTile,
            std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter) = 0;

        virtual bool canSymbolsBeSharedFrom(const std::shared_ptr<const Model::MapObject>& mapObject);
    };

}

#endif // _OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_
