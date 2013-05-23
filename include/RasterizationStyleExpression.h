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

#ifndef __RENDER_STYLE_EXPRESSION_H_
#define __RENDER_STYLE_EXPRESSION_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QStringList>
#include <QFile>
#include <QXmlStreamReader>
#include <QHash>

#include <OsmAndCore.h>
#include <RasterizationStyleExpression.h>

namespace OsmAnd {

    class OSMAND_CORE_API RasterizationStyleExpression
    {
    public:
        enum Type
        {
            String,
            Boolean,
            Integer,
            Float,
            Color,
        };
    private:
    protected:
        RasterizationStyleExpression(Type type, const QString& name);

        virtual bool evaluate() const = 0;
    public:
        virtual ~RasterizationStyleExpression();

        const Type type;
        const QString name;

    friend class RasterizationStyle;
    };


} // namespace OsmAnd

#endif // __RENDER_STYLE_EXPRESSION_H_
