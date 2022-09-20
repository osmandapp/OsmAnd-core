#ifndef _OSMAND_CORE_UNRESOLVED_MAP_STYLE_P_H_
#define _OSMAND_CORE_UNRESOLVED_MAP_STYLE_P_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include <QXmlStreamReader>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QAtomicInt>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "UnresolvedMapStyle.h"
#include "MapStyleConstantValue.h"

static const QString SEQ_PLACEHOLDER = QStringLiteral("#SEQ");
static const QString SEQ_ATTR = QStringLiteral("seq");
static const QString ORDER_BY_DENSITY_ATTR = QStringLiteral("orderByDensity");
static const QString ONEWAY_ARROWS_COLOR_ATTR = QStringLiteral("onewayArrowsColor");
static const QString ADD_POINT_ATTR = QStringLiteral("addPoint");

namespace OsmAnd
{
    class UnresolvedMapStyle;
    struct XmlTreeSequence;
    class UnresolvedMapStyle_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(UnresolvedMapStyle_P);
        
    public:
        typedef UnresolvedMapStyle::RuleNode RuleNode;
        typedef UnresolvedMapStyle::BaseRule BaseRule;
        typedef UnresolvedMapStyle::Rule Rule;
        typedef UnresolvedMapStyle::RulesByTagValueCollection RulesByTagValueCollection;
        typedef UnresolvedMapStyle::Attribute Attribute;
        typedef UnresolvedMapStyle::Parameter Parameter;

        typedef QHash< QString, QHash<QString, std::shared_ptr< Rule > > > EditableRulesByTagValueCollection;
    private:
        bool parseTagStart_RenderingStyle(QXmlStreamReader& xmlReader);

        const std::shared_ptr<QIODevice> _source;
        
        QHash<QString, QString> constants;
        QList< std::shared_ptr<const Parameter> > parameters;
        QList< std::shared_ptr<const Attribute> > attributes;
        std::array<EditableRulesByTagValueCollection, MapStyleRulesetTypesCount> rulesets;
        
        QAtomicInt _isMetadataLoaded;
        mutable QMutex _metadataLoadMutex;
        bool parseMetadata();
        bool parseMetadata(QXmlStreamReader& xmlReader);

        QAtomicInt _isLoaded;
        mutable QMutex _loadMutex;
        
        bool inSequence = false;
        
        bool parse();
        
        bool processStartElement(OsmAnd::MapStyleRulesetType &currentRulesetType,
                            QStack<std::shared_ptr<RuleNode> > &ruleNodesStack,
                            const QString &tagName,
                            const QXmlStreamAttributes &attribs,
                            qint64 lineNum, qint64 columnNum);
        
        bool processEndElement(OsmAnd::MapStyleRulesetType &currentRulesetType,
                            QStack<std::shared_ptr<RuleNode> > &ruleNodesStack,
                            const QString &tagName,
                            qint64 lineNum, qint64 columnNum);
        
        bool parse(QXmlStreamReader& xmlReader);

        static bool insertNodeIntoTopLevelTagValueRule(
            EditableRulesByTagValueCollection& rules,
            const MapStyleRulesetType rulesetType,
            const std::shared_ptr<RuleNode>& ruleNode);
    protected:
        UnresolvedMapStyle_P(
            UnresolvedMapStyle* const owner,
            const std::shared_ptr<QIODevice>& source,
            const QString& name);

        QString _title;

        const QString _name;
        QString _parentName;

        QHash<QString, QString> _constants;
        QList< std::shared_ptr<const Parameter> > _parameters;
        QList< std::shared_ptr<const Attribute> > _attributes;
        std::array<RulesByTagValueCollection, MapStyleRulesetTypesCount> _rulesets;
    public:
        ~UnresolvedMapStyle_P();

        ImplementationInterface<UnresolvedMapStyle> owner;

        bool isMetadataLoaded() const;
        bool loadMetadata();

        bool isStandalone() const;

        bool isLoaded() const;
        bool load();

    friend class OsmAnd::UnresolvedMapStyle;
    friend struct OsmAnd::XmlTreeSequence;
    };

    struct XmlTreeSequence {
        QString seqOrder;
        QXmlStreamAttributes attrsMap;
        QString name;
        std::vector<std::weak_ptr<XmlTreeSequence>> children;
        std::weak_ptr<XmlTreeSequence> parent;
        qint64 lineNum;
        qint64 columnNum;
        
        void process(int i,
                     UnresolvedMapStyle_P *parserObj,
                     OsmAnd::MapStyleRulesetType &currentRulesetType,
                     QStack<std::shared_ptr<UnresolvedMapStyle::RuleNode> > &ruleNodesStack);
    };

}

#endif // !defined(_OSMAND_CORE_UNRESOLVED_MAP_STYLE_P_H_)
