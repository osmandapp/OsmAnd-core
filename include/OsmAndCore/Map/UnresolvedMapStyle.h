#ifndef _OSMAND_CORE_UNRESOLVED_MAP_STYLE_H_
#define _OSMAND_CORE_UNRESOLVED_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QStringList>
#include <QIODevice>
#include <QByteArray>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class UnresolvedMapStyle_P;
    class OSMAND_CORE_API UnresolvedMapStyle
    {
        Q_DISABLE_COPY_AND_MOVE(UnresolvedMapStyle);
    public:
        class OSMAND_CORE_API RuleNode Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(RuleNode);

        private:
        protected:
        public:
            RuleNode(const bool isSwitch);
            ~RuleNode();

            const bool isSwitch;

            QHash<QString, QString> values;
            QList< std::shared_ptr<SWIG_CLARIFY(UnresolvedMapStyle, RuleNode)> > oneOfConditionalSubnodes;
            QList< std::shared_ptr<SWIG_CLARIFY(UnresolvedMapStyle, RuleNode)> > applySubnodes;
        };

        class OSMAND_CORE_API BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(BaseRule);

        private:
        protected:
            BaseRule(RuleNode* const ruleNode);
        public:
            virtual ~BaseRule();

            const std::shared_ptr<RuleNode> rootNode;
        };

        class OSMAND_CORE_API Rule Q_DECL_FINAL : public BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(Rule);

        private:
        protected:
        public:
            Rule(const MapStyleRulesetType rulesetType);
            virtual ~Rule();

            const MapStyleRulesetType rulesetType;
        };
        typedef QHash< QString, QHash<QString, std::shared_ptr< const Rule > > > RulesByTagValueCollection;

        class OSMAND_CORE_API Attribute Q_DECL_FINAL : public BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(Attribute);

        private:
        protected:
        public:
            Attribute(const QString& name);
            virtual ~Attribute();

            const QString name;
        };

        class OSMAND_CORE_API Parameter Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(Parameter);

        private:
        protected:
        public:
            Parameter(
                const QString& title,
                const QString& description,
                const QString& category,
                const QString& name,
                const MapStyleValueDataType dataType,
                const QStringList& possibleValues,
                const QString& defaultValueDescription);
            ~Parameter();

            QString title;
            QString description;
            QString category;
            QString name;
            MapStyleValueDataType dataType;
            QStringList possibleValues;
            QString defaultValueDescription;
        };

    private:
        PrivateImplementation<UnresolvedMapStyle_P> _p;
    protected:
    public:
        UnresolvedMapStyle(const std::shared_ptr<QIODevice>& source, const QString& name);
        UnresolvedMapStyle(const QString& fileName, const QString& name = {});
        virtual ~UnresolvedMapStyle();

        bool isMetadataLoaded() const;
        bool loadMetadata();

        const QString& title;
        const QString& name;
        const QString& parentName;

        const QHash<QString, QString>& constants;
        const QList< std::shared_ptr<const Parameter> >& parameters;
        const QList< std::shared_ptr<const Attribute> >& attributes;
#if !defined(SWIG)
        const std::array<RulesByTagValueCollection, MapStyleRulesetTypesCount>& rulesets;
#endif // !defined(SWIG)

        bool isStandalone() const;

        bool isLoaded() const;
        bool load();
    };
}

#endif // !defined(_OSMAND_CORE_UNRESOLVED_MAP_STYLE_H_)
