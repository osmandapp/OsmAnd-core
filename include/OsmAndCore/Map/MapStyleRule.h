#ifndef _OSMAND_CORE_MAP_STYLE_RULE_H_
#define _OSMAND_CORE_MAP_STYLE_RULE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class MapStyle;
    class MapStyle_P;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class MapStyleValueDefinition;
    struct MapStyleValue;

    class MapStyleRule_P;
    class OSMAND_CORE_API MapStyleRule
    {
        Q_DISABLE_COPY(MapStyleRule);
    private:
        const std::unique_ptr<MapStyleRule_P> _d;
    protected:
        MapStyleRule(MapStyle* owner, const QHash< QString, QString >& attributes);

        bool getAttribute(const std::shared_ptr<const MapStyleValueDefinition>& key, MapStyleValue& value) const;
    public:
        virtual ~MapStyleRule();

        MapStyle* const owner;

        void dump(const QString& prefix = QString()) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_RULE_H_)
