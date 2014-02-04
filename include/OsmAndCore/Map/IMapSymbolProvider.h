/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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
#include <QVector>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapProvider.h>
#include <OsmAndCore/Map/IRetainableResource.h>
#include <OsmAndCore/Data/Model/MapObject.h>

namespace OsmAnd
{
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

    class OSMAND_CORE_API MapSymbol : public IRetainableResource
    {
        Q_DISABLE_COPY(MapSymbol);
    private:
    protected:
        std::shared_ptr<const SkBitmap> _bitmap;

        MapSymbol(
            const std::weak_ptr<const MapSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order);
    public:
        virtual ~MapSymbol();

        const std::weak_ptr<const MapSymbolsGroup> group;

        const std::shared_ptr<const Model::MapObject> mapObject;
        const std::shared_ptr<const SkBitmap>& bitmap;
        const int order;

        virtual void releaseNonRetainedData();
        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const = 0;
    };

    class OSMAND_CORE_API MapPinnedSymbol : public MapSymbol
    {
        Q_DISABLE_COPY(MapPinnedSymbol);
    private:
    protected:
    public:
        MapPinnedSymbol(
            const std::weak_ptr<const MapSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const PointI& location31,
            const PointI& offset);
        virtual ~MapPinnedSymbol();

        const PointI location31;
        const PointI offset;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };

    class OSMAND_CORE_API MapSymbolOnPath : public MapSymbol
    {
        Q_DISABLE_COPY(MapSymbolOnPath);
    private:
    protected:
    public:
        MapSymbolOnPath(
            const std::weak_ptr<const MapSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QVector<float>& glyphsWidth);
        virtual ~MapSymbolOnPath();

        const QVector<float> glyphsWidth;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
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
