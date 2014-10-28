#ifndef _OSMAND_CORE_TOOLS_PRODUCTIZER_H_
#define _OSMAND_CORE_TOOLS_PRODUCTIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <iostream>
#include <sstream>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QStringList>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/WorldRegions.h>

#include <OsmAndCoreTools.h>

namespace OsmAndTools
{
    class OSMAND_CORE_TOOLS_API Productizer Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(Productizer);

    public:
        struct OSMAND_CORE_TOOLS_API Configuration Q_DECL_FINAL
        {
            Configuration();

            QHash< QString, std::shared_ptr<const OsmAnd::WorldRegions::WorldRegion> > regions;
            QString outputProductsFilename;
            bool verbose;

            static bool parseFromCommandLineArguments(
                const QStringList& commandLineArgs,
                Configuration& outConfiguration,
                QString& outError);
        };

    private:
#if defined(_UNICODE) || defined(UNICODE)
        bool productize(std::wostream& output);
#else
        bool productize(std::ostream& output);
#endif
    protected:
    public:
        Productizer(const Configuration& configuration);
        ~Productizer();

        const Configuration configuration;

        bool productize(QString *pLog = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_TOOLS_PRODUCTIZER_H_)
