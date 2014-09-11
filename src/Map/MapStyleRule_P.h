#ifndef _OSMAND_CORE_MAP_STYLE_RULE_P_H_
#define _OSMAND_CORE_MAP_STYLE_RULE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QHash>
#include <QList>

#include "OsmAndCore.h"
#include "MapStyleValue.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class MapStyle_P;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;

    class MapStyleNode;
    class MapStyleRule_P Q_DECL_FINAL
    {
    private:
    protected:
        MapStyleRule_P(MapStyleRule* owner);

        ImplementationInterface<MapStyleRule> owner;

        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _resolvedValueDefinitions;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > _values;
        QList< std::shared_ptr<const MapStyleRule> > _ifElseChildren;
        QList< std::shared_ptr<const MapStyleRule> > _ifChildren;
    public:
        virtual ~MapStyleRule_P();

    friend class OsmAnd::MapStyleNode;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_RULE_P_H_)
