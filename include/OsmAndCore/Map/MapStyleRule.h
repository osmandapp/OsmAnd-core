#ifndef _OSMAND_CORE_MAP_STYLE_RULE_H_
#define _OSMAND_CORE_MAP_STYLE_RULE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class MapStyle;
    class MapStyleEvaluator;
    class MapStyleValueDefinition;
    struct MapStyleValue;

    class MapStyleRule_P;
    class OSMAND_CORE_API MapStyleNode Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleRule);

    public:
        enum class RuleType
        {

        };

    private:
        PrivateImplementation<MapStyleRule_P> _p;
    protected:
        MapStyleRule(MapStyle* const style, const QHash< QString, QString >& attributes);
    public:
        virtual ~MapStyleRule();

        const MapStyle* const style;

        bool getAttribute(const std::shared_ptr<const MapStyleValueDefinition>& key, MapStyleValue& outValue) const;
        void dump(const QString& prefix = QString::null) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyleEvaluator;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_RULE_H_)
