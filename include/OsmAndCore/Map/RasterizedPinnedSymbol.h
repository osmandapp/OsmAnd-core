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

#ifndef _OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_
#define _OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterizedSymbol.h>

namespace OsmAnd
{
    class Rasterizer_P;

    class OSMAND_CORE_API RasterizedPinnedSymbol : public RasterizedSymbol
    {
        Q_DISABLE_COPY(RasterizedPinnedSymbol);
    private:
    protected:
        RasterizedPinnedSymbol(
            const std::shared_ptr<const RasterizedSymbolsGroup>& group,
            const std::shared_ptr<const Model::MapObject>& mapObject,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const PointI& location31,
            const PointI& offset);
    public:
        virtual ~RasterizedPinnedSymbol();

        const PointI location31;
        const PointI offset;

        friend class OsmAnd::Rasterizer_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_RASTERIZED_PINNED_SYMBOL_H_
