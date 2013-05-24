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

#ifndef __RASTERIZATION_STYLES_H_
#define __RASTERIZATION_STYLES_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QFile>
#include <QHash>

#include <OsmAndCore.h>

namespace OsmAnd {

    class RasterizationStyle;

    class OSMAND_CORE_API RasterizationStyles
    {
    private:
        bool registerEmbeddedStyle(const QString& resourceName);
    protected:
        QHash< QString, std::shared_ptr<RasterizationStyle> > _styles;
    public:
        RasterizationStyles();
        virtual ~RasterizationStyles();

        bool registerStyle(const QFile& file);
        bool obtainStyle(const QString& name, std::shared_ptr<OsmAnd::RasterizationStyle>& outStyle);

    friend class OsmAnd::RasterizationStyle;
    };

} // namespace OsmAnd

#endif // __RASTERIZATION_STYLES_H_
