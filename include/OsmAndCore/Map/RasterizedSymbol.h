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

#ifndef __RASTERIZED_SYMBOL_H_
#define __RASTERIZED_SYMBOL_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

class SkBitmap;

namespace OsmAnd {

    namespace Model {
        class MapObject;
    } // namespace Model
    class Rasterizer_P;

    class OSMAND_CORE_API RasterizedSymbol
    {
        Q_DISABLE_COPY(RasterizedSymbol);
    private:
    protected:
        RasterizedSymbol(const std::shared_ptr<const Model::MapObject>& mapObject, const std::shared_ptr<const SkBitmap>& iconBitmap);
    public:
        virtual ~RasterizedSymbol();

        const std::shared_ptr<const Model::MapObject> mapObject;

        const std::shared_ptr<const SkBitmap> iconBitmap;

    friend class OsmAnd::Rasterizer_P;
    };

} // namespace OsmAnd

#endif // __RASTERIZED_SYMBOL_H_
