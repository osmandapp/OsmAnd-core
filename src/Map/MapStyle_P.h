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
    class MapStylesCollection_P;
    class MapStyleValueDefinition;
    class MapStyleRule;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapStyle;
    class MapStyle_P Q_DECL_FINAL
    {
    private:
        bool parseMetadata(QXmlStreamReader& xmlReader);
        bool parse(QXmlStreamReader& xmlReader);

        void registerBuiltinValueDefinitions();
        void registerBuiltinValueDefinition(const std::shared_ptr<const MapStyleValueDefinition>& pValueDefinition);
        std::shared_ptr<const MapStyleValueDefinition> registerValue(const MapStyleValueDefinition* pValueDefinition);
        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _valuesDefinitions;
        int _firstNonBuiltinValueDefinitionIndex;

        bool registerRule(MapStyleRulesetType type, const std::shared_ptr<MapStyleRule>& rule);

        QHash< QString, QString > _parsetimeConstants;
        QHash< QString, std::shared_ptr<MapStyleRule> > _attributes;

        volatile bool _isMetadataLoaded;
        mutable QMutex _metadataLoadMutex;

        volatile bool _isLoaded;
        mutable QMutex _loadMutex;
    protected:
        MapStyle_P(MapStyle* const owner, const QString& name, const std::shared_ptr<QIODevice>& source);

        const std::shared_ptr<QIODevice> _source;

        bool parseMetadata();
        bool parse();

        QString _title;

        const QString _name;
        QString _parentName;
        std::shared_ptr<const MapStyle> _parent;

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

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        static uint64_t encodeRuleId(uint32_t tag, uint32_t value);

        const QMap< uint64_t, std::shared_ptr<MapStyleRule> >& obtainRulesRef(MapStyleRulesetType type) const;

        uint32_t getTagStringId(uint64_t ruleId) const;
        uint32_t getValueStringId(uint64_t ruleId) const;
        const QString& getTagString(uint64_t ruleId) const;
        const QString& getValueString(uint64_t ruleId) const;

        bool lookupStringId(const QString& value, uint32_t& id) const;
        const QString& lookupStringValue(uint32_t id) const;
    public:
        virtual ~MapStyle_P();

        ImplementationInterface<MapStyle> owner;

        bool isMetadataLoaded() const;
        bool loadMetadata();

        bool isStandalone() const;

        bool isLoaded() const;
        bool load();

        bool resolveValueDefinition(const QString& name, std::shared_ptr<const MapStyleValueDefinition>& outDefinition) const;
        bool resolveAttribute(const QString& name, std::shared_ptr<const MapStyleRule>& outAttribute) const;

        void dump(const QString& prefix) const;
        void dump(const MapStyleRulesetType type, const QString& prefix) const;

        bool parseValue(const std::shared_ptr<const MapStyleValueDefinition>& valueDef, const QString& input, MapStyleValue& output) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::MapStylesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_P_H_)
