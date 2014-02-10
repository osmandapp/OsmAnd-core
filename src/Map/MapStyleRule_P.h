#ifndef _OSMAND_CORE_MAP_STYLE_RULE_P_H_
#define _OSMAND_CORE_MAP_STYLE_RULE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QHash>
#include <QList>

#include "OsmAndCore.h"
#include "MapStyleValue.h"

namespace OsmAnd
{
    class MapStyle_P;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;

    class MapStyleRule;
    class MapStyleRule_P
    {
    private:
    protected:
        MapStyleRule_P(MapStyleRule* owner);

        MapStyleRule* const owner;

        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _resolvedValueDefinitions;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > _values;
        QList< std::shared_ptr<MapStyleRule> > _ifElseChildren;
        QList< std::shared_ptr<MapStyleRule> > _ifChildren;
    public:
        virtual ~MapStyleRule_P();

    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_RULE_P_H_)
