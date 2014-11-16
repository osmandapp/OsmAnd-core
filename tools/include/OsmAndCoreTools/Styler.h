#ifndef _OSMAND_CORE_TOOLS_STYLER_H_
#define _OSMAND_CORE_TOOLS_STYLER_H_

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
    class OSMAND_CORE_TOOLS_API Styler Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Styler);

    public:
        struct OSMAND_CORE_TOOLS_API Configuration Q_DECL_FINAL
        {
            Configuration();

            std::shared_ptr<OsmAnd::IObfsCollection> obfsCollection;
            std::shared_ptr<OsmAnd::IMapStylesCollection> stylesCollection;
            QString styleName;
            QHash< QString, QString > styleSettings;
            QSet< uint64_t > mapObjectsIds;
            OsmAnd::ZoomLevel zoom;
            float displayDensityFactor;
            QString locale;
            QString styleDumpFilename;
            bool verbose;

            static bool parseFromCommandLineArguments(
                const QStringList& commandLineArgs,
                Configuration& outConfiguration,
                QString& outError);
        };

        typedef QHash< std::shared_ptr<const OsmAnd::MapObject>, std::shared_ptr<const OsmAnd::MapPrimitiviser::PrimitivesGroup> > EvaluatedMapObjects;

    private:
#if defined(_UNICODE) || defined(UNICODE)
        bool evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, std::wostream& output);
#else
        bool evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, std::ostream& output);
#endif
    protected:
    public:
        Styler(const Configuration& configuration);
        ~Styler();

        const Configuration configuration;

        bool evaluate(EvaluatedMapObjects& outEvaluatedMapObjects, QString *pLog = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_TOOLS_STYLER_H_)
