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

#ifndef __ROUTING_RULESET_CONTEXT_H_
#define __ROUTING_RULESET_CONTEXT_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QHash>
#include <QBitArray>

#include <OsmAndCore.h>
#include <OsmAndCore/Routing/RoutingRuleset.h>

namespace OsmAnd {

    class RoutingProfileContext;
    class ObfRoutingSectionInfo;
    namespace Model {
        class Road;
    } // namespace Model

    class OSMAND_CORE_API RoutingRulesetContext
    {
    private:
        QHash<QString, QString> _contextValues;
        std::shared_ptr<RoutingRuleset> _ruleset;
    protected:
        bool evaluate(const std::shared_ptr<const Model::Road>& road, RoutingRuleExpression::ResultType type, void* result);
        bool evaluate(const QBitArray& types, RoutingRuleExpression::ResultType type, void* result);
        QBitArray encode(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes);
    public:
        RoutingRulesetContext(RoutingProfileContext* owner, const std::shared_ptr<RoutingRuleset>& ruleset, QHash<QString, QString>* contextValues);
        virtual ~RoutingRulesetContext();

        RoutingProfileContext* const owner;
        const std::shared_ptr<RoutingRuleset> ruleset;
        const QHash<QString, QString>& contextValues;

        int evaluateAsInteger(const std::shared_ptr<const Model::Road>& road, int defaultValue);
        float evaluateAsFloat(const std::shared_ptr<const Model::Road>& road, float defaultValue);

        int evaluateAsInteger(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes, int defaultValue);
        float evaluateAsFloat(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes, float defaultValue);
    };

} // namespace OsmAnd

#endif // __ROUTING_RULESET_CONTEXT_H_
