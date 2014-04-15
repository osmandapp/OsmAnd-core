#ifndef _OSMAND_CORE_MAP_STYLE_P_H_
#define _OSMAND_CORE_MAP_STYLE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QXmlStreamReader>
#include <QHash>
#include <QMap>

#include "OsmAndCore.h"
#include "MapStyle.h"
#include "MapStyleValue.h"

namespace OsmAnd
{
    class MapStyles_P;
    class MapStyleValueDefinition;
    class MapStyleRule;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapStyle;
    class MapStyle_P
    {
    private:
        bool parseMetadata(QXmlStreamReader& xmlReader);
        bool parse(QXmlStreamReader& xmlReader);

        void registerBuiltinValueDefinitions();
        void registerBuiltinValueDefinition(const std::shared_ptr<const MapStyleValueDefinition>& pValueDefinition);
        std::shared_ptr<const MapStyleValueDefinition> registerValue(const MapStyleValueDefinition* pValueDefinition);
        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _valuesDefinitions;
        uint32_t _firstNonBuiltinValueDefinitionIndex;

        bool registerRule(MapStyleRulesetType type, const std::shared_ptr<MapStyleRule>& rule);

        QHash< QString, QString > _parsetimeConstants;
        QHash< QString, std::shared_ptr<MapStyleRule> > _attributes;

        bool _isPrepared;
        mutable QMutex _preparationMutex;
    protected:
        MapStyle_P(MapStyle* owner);

        bool parseMetadata();
        bool parse();

        QString _title;

        QString _name;
        QString _parentName;
        std::shared_ptr<const MapStyle> _parent;

        bool prepareIfNeeded();

        bool areDependenciesResolved() const;
        bool resolveDependencies();
        bool resolveConstantValue(const QString& name, QString& value) const;
        QString obtainValue(const QString& value);

        bool mergeInherited();
        bool mergeInheritedRules(MapStyleRulesetType type);
        bool mergeInheritedAttributes();

        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _pointRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _lineRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _polygonRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _textRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _orderRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> >& obtainRulesRef(MapStyleRulesetType type);

        std::shared_ptr<MapStyleRule> createTagValueRootWrapperRule(uint64_t id, const std::shared_ptr<MapStyleRule>& rule);

        uint32_t _stringsIdBase;
        QList< QString > _stringsLUT;
        QHash< QString, uint32_t > _stringsRevLUT;
        uint32_t lookupStringId(const QString& value);
        uint32_t registerString(const QString& value);

        bool parseValue(const std::shared_ptr<const MapStyleValueDefinition>& valueDef, const QString& input, MapStyleValue& output, bool allowStringRegistration);

        enum {
            RuleIdTagShift = 32,
        };

        ImplementationInterface<MapStyle> owner;

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        static uint64_t encodeRuleId(uint32_t tag, uint32_t value);
    public:
        virtual ~MapStyle_P();

        const QMap< uint64_t, std::shared_ptr<MapStyleRule> >& obtainRulesRef(MapStyleRulesetType type) const;

        uint32_t getTagStringId(uint64_t ruleId) const;
        uint32_t getValueStringId(uint64_t ruleId) const;
        const QString& getTagString(uint64_t ruleId) const;
        const QString& getValueString(uint64_t ruleId) const;

        bool lookupStringId(const QString& value, uint32_t& id) const;
        const QString& lookupStringValue(uint32_t id) const;

        bool parseValue(const std::shared_ptr<const MapStyleValueDefinition>& valueDef, const QString& input, MapStyleValue& output) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::MapStyles_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_P_H_)
