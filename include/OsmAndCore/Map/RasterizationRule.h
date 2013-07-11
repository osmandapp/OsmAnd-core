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

#ifndef __RASTERIZATION_RULE_H_
#define __RASTERIZATION_RULE_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QHash>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/RasterizationStyle.h>

namespace OsmAnd {

    class RasterizationStyle;
    class RasterizationStyleEvaluator;

    class OSMAND_CORE_API RasterizationRule
    {
    public:
        union OSMAND_CORE_API Value
        {
            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };

    private:
    protected:
        RasterizationRule(RasterizationStyle* owner, const QHash< QString, QString >& attributes);

        QList< std::shared_ptr<RasterizationStyle::ValueDefinition> > _valueDefinitionsRefs;
        QHash< QString, Value > _values;
        QList< std::shared_ptr<RasterizationRule> > _ifElseChildren;
        QList< std::shared_ptr<RasterizationRule> > _ifChildren;
    public:
        virtual ~RasterizationRule();

        RasterizationStyle* const owner;

        const QString& getStringAttribute(const QString& key, bool* ok = nullptr) const;
        int getIntegerAttribute(const QString& key, bool* ok = nullptr) const;
        float getFloatAttribute(const QString& key, bool* ok = nullptr) const;

        void dump(const QString& prefix = QString()) const;

    friend class OsmAnd::RasterizationStyle;
    friend class OsmAnd::RasterizationStyleEvaluator;
    };


} // namespace OsmAnd

#endif // __RASTERIZATION_RULE_H_
