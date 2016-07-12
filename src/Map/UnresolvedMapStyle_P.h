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

namespace OsmAnd
{
    class UnresolvedMapStyle;
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

        QAtomicInt _isMetadataLoaded;
        mutable QMutex _metadataLoadMutex;
        bool parseMetadata();
        bool parseMetadata(QXmlStreamReader& xmlReader);

        QAtomicInt _isLoaded;
        mutable QMutex _loadMutex;
        bool parse();
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
    };
}

#endif // !defined(_OSMAND_CORE_UNRESOLVED_MAP_STYLE_P_H_)
