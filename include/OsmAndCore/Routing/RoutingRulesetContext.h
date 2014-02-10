#ifndef _OSMAND_CORE_ROUTING_RULESET_CONTEXT_H_
#define _OSMAND_CORE_ROUTING_RULESET_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
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
        bool evaluate(const std::shared_ptr<const Model::Road>& road, const RoutingRuleExpression::ResultType type, void* const result);
        bool evaluate(const QBitArray& types, const RoutingRuleExpression::ResultType type, void* const result);
        QBitArray encode(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes);
    public:
        RoutingRulesetContext(RoutingProfileContext* owner, const std::shared_ptr<RoutingRuleset>& ruleset, QHash<QString, QString>* const contextValues);
        virtual ~RoutingRulesetContext();

        RoutingProfileContext* const owner;
        const std::shared_ptr<RoutingRuleset> ruleset;
        const QHash<QString, QString>& contextValues;

        int evaluateAsInteger(const std::shared_ptr<const Model::Road>& road, const int defaultValue);
        float evaluateAsFloat(const std::shared_ptr<const Model::Road>& road, const float defaultValue);

        int evaluateAsInteger(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes, const int defaultValue);
        float evaluateAsFloat(const std::shared_ptr<const ObfRoutingSectionInfo>& section, const QVector<uint32_t>& roadTypes, const float defaultValue);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTING_RULESET_CONTEXT_H_)
