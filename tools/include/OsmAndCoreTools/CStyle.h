#ifndef _OSMAND_CORE_TOOLS_CSTYLE_H_
#define _OSMAND_CORE_TOOLS_CSTYLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iostream>
#include <sstream>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Data/BinaryMapObject.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>
#include <OsmAndCore/Map/MapPrimitiviser.h>

#include <OsmAndCoreTools.h>

namespace OsmAndTools
{
    class OSMAND_CORE_TOOLS_API CStyle Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(CStyle);

    public:
        enum class EmitterDialect
        {
            Debug,
        };

        struct OSMAND_CORE_TOOLS_API Configuration Q_DECL_FINAL
        {
            Configuration();

            std::shared_ptr<OsmAnd::IMapStylesCollection> stylesCollection;
            QString styleName;
            EmitterDialect emitterDialect;
            bool verbose;

            static bool parseFromCommandLineArguments(
                const QStringList& commandLineArgs,
                Configuration& outConfiguration,
                QString& outError);
        };

        struct OSMAND_CORE_TOOLS_API BaseEmitter
        {
#if defined(_UNICODE) || defined(UNICODE)
            BaseEmitter(std::wostream& output);
#else
            BaseEmitter(std::ostream& output);
#endif
            virtual ~BaseEmitter();

#if defined(_UNICODE) || defined(UNICODE)
            std::wostream& output;
#else
            std::ostream& output;
#endif

            virtual bool emitStyle(const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle) = 0;

        private:
            Q_DISABLE_COPY_AND_MOVE(BaseEmitter);
        };

        struct OSMAND_CORE_TOOLS_API DebugEmitter : public BaseEmitter
        {
#if defined(_UNICODE) || defined(UNICODE)
            DebugEmitter(std::wostream& output);
#else
            DebugEmitter(std::ostream& output);
#endif
            virtual ~DebugEmitter();

            virtual bool emitStyle(const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle) Q_DECL_OVERRIDE;

        private:
            Q_DISABLE_COPY_AND_MOVE(DebugEmitter);

            static QString dumpConstantValue(
                const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
                const OsmAnd::MapStyleConstantValue& value,
                const OsmAnd::MapStyleValueDataType dataType);
            static QString dumpRuleNode(
                const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
                const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& ruleNode,
                const bool rejectSupported,
                const QString& prefix);
            static QString dumpResolvedValue(
                const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
                const OsmAnd::IMapStyle::Value& value,
                const OsmAnd::MapStyleValueDataType dataType);
            static QString dumpRuleNodeOutputValues(
                const std::shared_ptr<const OsmAnd::IMapStyle>& mapStyle,
                const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& ruleNode,
                const QString& prefix,
                const bool allowOverride);
        };

    private:
#if defined(_UNICODE) || defined(UNICODE)
        bool compile(std::wostream& output);
#else
        bool compile(std::ostream& output);
#endif
    protected:
    public:
        CStyle(const Configuration& configuration);
        ~CStyle();

        const Configuration configuration;

        bool compile(QString *pLog = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_TOOLS_CSTYLE_H_)
